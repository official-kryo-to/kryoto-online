// ============================================================
// kryotoO.dll - every patch KryotoOnline applies.
//
// This is the half that writes to other modules' code. It links
// MinHook; the steam_api proxy no longer does, and that is the
// point of the split - the file whose export table has to match
// Valve's byte for byte is now pure forwarding, and a hook fix
// ships as one small DLL dropped next to it.
//
// Three patches live here:
//
//   1. SteamStub  Valve's executable wrapper checks, through the
//                 live client, that the account owns the REAL
//                 AppId. Under the Spacewar spoof it does not, so
//                 the stub refuses before the game reaches its
//                 entry point. This flips the branch it makes
//                 that decision on. Opt-in (GetStubbedLol),
//                 because the packaged path strips the stub with
//                 Steamless instead and a patch looking for a
//                 signature that is not there is only overhead.
//
//   2. GetAppID   ISteamUtils::GetAppID reports Spacewar, since
//                 that is genuinely what the client thinks is
//                 running. Games that build their own state off
//                 it then talk to the wrong title's backend.
//                 Report ogAppId instead.
//
//   3. BIsSubscribedApp
//                 Games gate multiplayer behind "does this
//                 account own me?". Real Steam answers no - the
//                 account owns 480. Answer yes for ogAppId only,
//                 so every other ownership question still gets
//                 the truth.
//
// 2 and 3 stand down entirely when there is no ogAppId, or when
// it equals AppId: there is no mapping to make, and hooking the
// live client to return what it already returns is pure risk.
// ============================================================

#include <Windows.h>
#include <intrin.h>
#include <stdarg.h>

#include <atomic>

#include "kryotoo_abi.h"
#include "kryotoo_config.h"
#include "kryotoo_plugins.h"

#include "../include/MinHook.h"

// ============================================================
// State
// ============================================================

static KryotoO_LogFn      g_Log = nullptr;
static kryotoo::Config    g_Config;
static kryotoo::Plugins   g_Plugins;
static std::vector<uint32_t> g_Dlc;

static uint32_t g_ForcedAppId = kryotoo::kSpacewarAppId;
static uint32_t g_OgAppId = 0;

static bool g_MinHookUp = false;
static bool g_StubHookOn = false;
static bool g_SpoofHooksOn = false;
static bool g_StartedUp = false;

/// Logging goes through the proxy so both halves land in one file
/// in one order. A null logger is normal when the core is loaded
/// by something that does not provide one; it must never crash.
static void Log(const char* fmt, ...)
{
    if (!g_Log)
        return;

    // The host's Log is printf-style and variadic, and there is no
    // portable way to forward a va_list into it. Format here and
    // hand it a finished string; "%s" keeps a stray percent in a
    // path or a plugin name from being read as a directive.
    char msg[2048] = { 0 };
    va_list args;
    va_start(args, fmt);
    _vsnprintf_s(msg, sizeof(msg), _TRUNCATE, fmt, args);
    va_end(args);

    g_Log("%s", msg);
}

/// MinHook is initialised once and shared by both patches.
static bool EnsureMinHook()
{
    if (g_MinHookUp)
        return true;

    MH_STATUS s = MH_Initialize();
    // ALREADY_INITIALIZED is success as far as this is concerned -
    // something else in the process got there first, and the hooks
    // below work either way.
    if (s != MH_OK && s != MH_ERROR_ALREADY_INITIALIZED)
    {
        Log("[KryotoOnline] MH_Initialize failed: %d", s);
        return false;
    }

    g_MinHookUp = true;
    return true;
}

// ============================================================
// Patch 1: SteamStub
//
// Original approach from DenuvoSanctuary, in Rust; this is the
// same idea rewritten to live inside the DLL that needs it.
//
// The stub calls GetTickCount as part of its check. Hook that,
// look back from the return address at the code that called us,
// and if the ownership comparison is sitting there, turn its
// JZ (0x84) into a JNZ (0x85). One site is enough - the stub
// asks once - so the hook disables itself immediately afterwards
// rather than scanning on every tick for the rest of the run.
// ============================================================

static constexpr uint8_t kStubSignature[] = { 0x44, 0x0F, 0xB6, 0xF8, 0x3C, 0x30, 0x0F, 0x84 };
/// How far back from the return address to look. The comparison is
/// within a few instructions of the call; a wider window only
/// raises the chance of matching something that is not the stub.
static constexpr size_t kStubScanBytes = 128;

typedef DWORD(WINAPI* GetTickCount_Fn)(void);
static GetTickCount_Fn g_OrigGetTickCount = nullptr;
static std::atomic<bool> g_StubPatched{ false };

static uint8_t* FindSignature(uint8_t* start, uint8_t* end, const uint8_t* sig, size_t len)
{
    if (!start || !end || end <= start || (size_t)(end - start) < len)
        return nullptr;

    for (uint8_t* p = start; p <= end - len; ++p)
    {
        if (memcmp(p, sig, len) == 0)
            return p;
    }
    return nullptr;
}

static DWORD WINAPI Hooked_GetTickCount(void)
{
    // Already done: fall straight through. This runs on the game's
    // hot path until the hook is removed, so it does no work.
    if (g_StubPatched.load(std::memory_order_acquire))
        return g_OrigGetTickCount();

    uint8_t* start = reinterpret_cast<uint8_t*>(_ReturnAddress());
    uint8_t* end = start + kStubScanBytes;

    DWORD oldProtect = 0;
    if (!VirtualProtect(start, kStubScanBytes, PAGE_EXECUTE_READWRITE, &oldProtect))
        return g_OrigGetTickCount();

    uint8_t* found = FindSignature(start, end, kStubSignature, sizeof(kStubSignature));
    if (found)
    {
        found[7] = 0x85;  // JZ -> JNZ: the ownership test now passes.
        g_StubPatched.store(true, std::memory_order_release);
    }

    VirtualProtect(start, kStubScanBytes, oldProtect, &oldProtect);

    if (found)
    {
        // Disabled, not removed: MH_RemoveHook frees the trampoline
        // this call is about to return through.
        MH_DisableHook(reinterpret_cast<LPVOID>(GetTickCount));
        Log("[KryotoOnline] SteamStub ownership branch patched");
    }

    return g_OrigGetTickCount();
}

static void InstallSteamStubPatch()
{
    if (!EnsureMinHook())
        return;

    LPVOID target = reinterpret_cast<LPVOID>(GetTickCount);
    MH_STATUS s = MH_CreateHook(target, &Hooked_GetTickCount,
                                reinterpret_cast<LPVOID*>(&g_OrigGetTickCount));
    if (s != MH_OK)
    {
        Log("[KryotoOnline] SteamStub: MH_CreateHook failed: %d", s);
        return;
    }

    s = MH_EnableHook(target);
    if (s != MH_OK)
    {
        Log("[KryotoOnline] SteamStub: MH_EnableHook failed: %d", s);
        return;
    }

    g_StubHookOn = true;
    Log("[KryotoOnline] SteamStub patch armed");
}

// ============================================================
// Patches 2 and 3: the ownership spoof
// ============================================================

typedef uint32_t(__cdecl* Fn_GetAppID)(void* self);
typedef bool(__cdecl* Fn_BIsSubscribedApp)(void* self, uint32_t appId);

static Fn_GetAppID         g_OrigGetAppID = nullptr;
static Fn_BIsSubscribedApp g_OrigBIsSubscribedApp = nullptr;
static bool                g_LoggedGetAppID = false;
static bool                g_LoggedSubscribed = false;

static uint32_t __cdecl Hooked_GetAppID(void* self)
{
    uint32_t real = g_OrigGetAppID(self);

    if (!g_LoggedGetAppID)
    {
        Log("[KryotoOnline] GetAppID -> %u (client says %u)", g_OgAppId, real);
        g_LoggedGetAppID = true;
    }
    return g_OgAppId;
}

static bool __cdecl Hooked_BIsSubscribedApp(void* self, uint32_t appId)
{
    bool owned = g_OrigBIsSubscribedApp(self, appId);

    // Only the one AppId, and only when the client said no. Every
    // other ownership question - real DLC, other games - keeps
    // answering truthfully, which matters because a game that gets
    // "yes" for everything behaves worse than one that gets "no".
    if (appId == g_OgAppId && !owned)
    {
        if (!g_LoggedSubscribed)
        {
            Log("[KryotoOnline] BIsSubscribedApp(%u) -> true (client says false)", appId);
            g_LoggedSubscribed = true;
        }
        return true;
    }
    return owned;
}

/// Hook one vtable slot. `index` is the slot; see the call site for
/// where the numbers come from.
static bool HookVTableSlot(void* iface, size_t index, void* detour, void** original, const char* what)
{
    if (!iface)
        return false;

    void** vtable = *reinterpret_cast<void***>(iface);
    void* target = vtable[index];

    MH_STATUS s = MH_CreateHook(target, detour, original);
    if (s != MH_OK)
    {
        Log("[KryotoOnline] %s: MH_CreateHook failed: %d", what, s);
        return false;
    }

    s = MH_EnableHook(target);
    if (s != MH_OK)
    {
        Log("[KryotoOnline] %s: MH_EnableHook failed: %d", what, s);
        return false;
    }

    Log("[KryotoOnline] %s hook installed", what);
    return true;
}

// ============================================================
// Exports
// ============================================================

extern "C" uint32_t __cdecl KryotoO_AbiVersion(void)
{
    return KRYOTOO_ABI_VERSION;
}

extern "C" int32_t __cdecl KryotoO_Startup(const KryotoO_Host* host, KryotoO_Config* outCfg)
{
    if (!host || host->StructSize != sizeof(KryotoO_Host))
        return 1;
    if (!outCfg || outCfg->StructSize != sizeof(KryotoO_Config))
        return 2;
    if (host->AbiVersion != KRYOTOO_ABI_VERSION)
        return 3;
    if (g_StartedUp)
        return 4;

    g_StartedUp = true;
    g_Log = host->Log;

    g_Config.Read();
    g_ForcedAppId = g_Config.AppId();
    g_OgAppId = g_Config.OgAppId();
    g_Dlc = g_Config.UnlockDlc();

    if (g_Config.HaveIni())
        Log("[KryotoOnline] Config: %s", g_Config.IniPath());
    else
        Log("[KryotoOnline] No kryoto-online.ini beside the executable; using defaults");

    Log("[KryotoOnline] AppId=%u ogAppId=%u dlc=%zu ticket=%d stub=%d",
        g_ForcedAppId, g_OgAppId, g_Dlc.size(),
        (int)g_Config.EmulateTicket(), (int)g_Config.SteamStubEnabled());

    outCfg->AppId = g_ForcedAppId;
    outCfg->OgAppId = g_OgAppId;
    outCfg->EmulateTicket = g_Config.EmulateTicket() ? 1 : 0;
    // What the ini asked for. The patch itself is armed later, in
    // KryotoO_LoadPlugins, so reporting `g_StubHookOn` here would
    // always answer 0.
    outCfg->SteamStubEnabled = g_Config.SteamStubEnabled() ? 1 : 0;
    outCfg->DlcCount = (uint32_t)g_Dlc.size();
    outCfg->Dlc = g_Dlc.empty() ? nullptr : g_Dlc.data();
    outCfg->HaveIni = g_Config.HaveIni() ? 1 : 0;

    return 0;
}

extern "C" uint32_t __cdecl KryotoO_LoadPlugins(void)
{
    if (!g_StartedUp)
        return 0;

    char folder[MAX_PATH] = { 0 };
    g_Config.PluginsFolder(folder, sizeof(folder));
    g_Plugins.Load(folder, &Log);
    Log("[KryotoOnline] %zu plugin(s) loaded", g_Plugins.Count());

    // AFTER the plugins, which is where it has always gone.
    //
    // The hook fires on every GetTickCount until it finds its signature,
    // and what it inspects is the CALLER's code. Arming it earlier would
    // point it at each plugin's DllMain on the way past - an eight-byte
    // signature is unlikely to collide, but the reward for winning that
    // gamble is writing a byte into somebody else's function.
    if (g_Config.SteamStubEnabled())
        InstallSteamStubPatch();

    return (uint32_t)g_Plugins.Count();
}

extern "C" void __cdecl KryotoO_InstallSpoofHooks(void* pSteamUtils, void* pSteamApps)
{
    if (g_SpoofHooksOn)
        return;

    if (g_OgAppId == 0 || g_OgAppId == g_ForcedAppId)
    {
        Log("[KryotoOnline] Spoof hooks not needed (ogAppId=%u AppId=%u)", g_OgAppId, g_ForcedAppId);
        return;
    }

    if (!EnsureMinHook())
        return;

    // ISteamUtils slot 9 = GetAppID:
    //   0 GetSecondsSinceAppActive   1 GetSecondsSinceComputerActive
    //   2 GetConnectedUniverse       3 GetServerRealTime
    //   4 GetIPCountry               5 GetImageSize
    //   6 GetImageRGBA               7 GetCSERIPPort (private, still in the vtable)
    //   8 GetCurrentBatteryPower     9 GetAppID
    bool utils = HookVTableSlot(pSteamUtils, 9, &Hooked_GetAppID,
                                reinterpret_cast<void**>(&g_OrigGetAppID), "GetAppID");

    // ISteamApps slot 6 = BIsSubscribedApp:
    //   0 BIsSubscribed         1 BIsLowViolence
    //   2 BIsCybercafe          3 BIsVACBanned
    //   4 GetCurrentGameLanguage 5 GetAvailableGameLanguages
    //   6 BIsSubscribedApp
    bool apps = HookVTableSlot(pSteamApps, 6, &Hooked_BIsSubscribedApp,
                               reinterpret_cast<void**>(&g_OrigBIsSubscribedApp), "BIsSubscribedApp");

    g_SpoofHooksOn = utils || apps;
}

extern "C" uint32_t __cdecl KryotoO_InitPlugins(const void* pluginCtx)
{
    if (!pluginCtx)
        return 0;
    return (uint32_t)g_Plugins.Init(
        reinterpret_cast<const KRYOTO_PluginContext*>(pluginCtx), &Log);
}

extern "C" void __cdecl KryotoO_Shutdown(void)
{
    g_Plugins.Shutdown();

    if (g_StubHookOn)
    {
        MH_DisableHook(reinterpret_cast<LPVOID>(GetTickCount));
        g_StubHookOn = false;
    }

    if (g_MinHookUp)
    {
        // Every hook at once. Individually disabling the vtable
        // slots would need the interface pointers again, and the
        // client may already be gone by the time this runs.
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
        g_MinHookUp = false;
    }

    g_SpoofHooksOn = false;
    g_Log = nullptr;
}

// ============================================================
// DllMain
//
// Does nothing. Everything happens in KryotoO_Startup, called
// explicitly by the proxy, because the work here loads libraries
// and patches code and none of that belongs under the loader
// lock any longer than it has to be.
// ============================================================

BOOL WINAPI DllMain(HMODULE module, DWORD reason, LPVOID reserved)
{
    (void)reserved;

    if (reason == DLL_PROCESS_ATTACH)
    {
        // Nothing here needs per-thread notifications, and a game
        // that spawns threads constantly pays for them.
        DisableThreadLibraryCalls(module);
    }
    return TRUE;
}
