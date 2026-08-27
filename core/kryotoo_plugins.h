// Plugin loading.
//
// A plugin is a DLL in the folder named by `PluginsFolder=`, holding
// the per-game work that has no business in a generic Steam wrapper:
// Photon backends, IL2CPP patches, auth-ticket synthesis. The ABI
// they implement is include/kryoto_plugin.h and it is unchanged by
// the core/proxy split - a plugin built against the old host still
// loads, because it never knew which module was calling it.
//
// Load order is alphabetical, deliberately: it is the only ordering
// a person can predict from a directory listing, so a plugin that
// must run before another can be named for it.
#pragma once

#include <Windows.h>
#include <Shlwapi.h>

#include <algorithm>
#include <string>
#include <vector>

#include "../include/kryoto_plugin.h"

namespace kryotoo {

class Plugins
{
public:
    /// Load every *.dll in `<exe dir>\<folder>`. A folder that is
    /// empty, missing or unnamed loads nothing and is not an error:
    /// most releases ship no plugins at all.
    template <typename LogFn>
    void Load(const char* folder, LogFn log)
    {
        if (!folder || folder[0] == '\0')
            return;

        char exeDir[MAX_PATH] = { 0 };
        if (GetModuleFileNameA(nullptr, exeDir, MAX_PATH) == 0)
            return;
        if (!PathRemoveFileSpecA(exeDir))
            return;

        char dir[MAX_PATH] = { 0 };
        if (_snprintf_s(dir, MAX_PATH, _TRUNCATE, "%s\\%s", exeDir, folder) == _TRUNCATE)
            return;

        DWORD attribs = GetFileAttributesA(dir);
        if (attribs == INVALID_FILE_ATTRIBUTES || !(attribs & FILE_ATTRIBUTE_DIRECTORY))
        {
            log("[KryotoOnline] Plugins folder not found: %s", dir);
            return;
        }

        char pattern[MAX_PATH] = { 0 };
        if (_snprintf_s(pattern, MAX_PATH, _TRUNCATE, "%s\\*.dll", dir) == _TRUNCATE)
            return;

        std::vector<std::string> names;

        WIN32_FIND_DATAA fd = { 0 };
        HANDLE find = FindFirstFileA(pattern, &fd);
        if (find == INVALID_HANDLE_VALUE)
            return;
        do
        {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                names.push_back(fd.cFileName);
        } while (FindNextFileA(find, &fd));
        FindClose(find);

        std::sort(names.begin(), names.end(), [](const std::string& a, const std::string& b) {
            return _stricmp(a.c_str(), b.c_str()) < 0;
        });

        for (const std::string& name : names)
        {
            char full[MAX_PATH] = { 0 };
            if (_snprintf_s(full, MAX_PATH, _TRUNCATE, "%s\\%s", dir, name.c_str()) == _TRUNCATE)
                continue;

            // LOAD_WITH_ALTERED_SEARCH_PATH so a plugin's own
            // dependencies resolve from the plugins folder rather
            // than from the game's root.
            HMODULE mod = LoadLibraryExA(full, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
            if (mod)
            {
                m_Modules.push_back(mod);
                m_Names.push_back(name);
                log("[KryotoOnline] Loaded plugin: %s", name.c_str());
            }
            else
            {
                log("[KryotoOnline] Failed to load plugin: %s (error %lu)", name.c_str(), GetLastError());
            }
        }
    }

    /// Hand every plugin its context. Returns how many reported
    /// success. A plugin that exports no init is still loaded - the
    /// ABI has always allowed a plugin to do its work from DllMain.
    template <typename LogFn>
    size_t Init(const KRYOTO_PluginContext* ctx, LogFn log)
    {
        size_t ok = 0;
        for (size_t i = 0; i < m_Modules.size(); i++)
        {
            auto init = (KRYOTO_PluginInit_Fn)GetProcAddress(m_Modules[i], "KRYOTO_PluginInit");
            if (!init)
                continue;

            int rc = init(ctx);
            if (rc == 0)
            {
                ok++;
                log("[KryotoOnline] Plugin init OK: %s", m_Names[i].c_str());
            }
            else
            {
                log("[KryotoOnline] Plugin init returned %d: %s", rc, m_Names[i].c_str());
            }
        }
        return ok;
    }

    /// Reverse load order, so a plugin can rely on the ones it was
    /// ordered after still being alive while it tears down.
    /// Idempotent: the game may call SteamAPI_Shutdown and then the
    /// process may detach.
    void Shutdown()
    {
        if (m_ShutdownCalled)
            return;
        m_ShutdownCalled = true;

        for (size_t n = m_Modules.size(); n > 0; --n)
        {
            auto shut = (KRYOTO_PluginShutdown_Fn)GetProcAddress(m_Modules[n - 1], "KRYOTO_PluginShutdown");
            if (shut)
                shut();
        }

        // Deliberately no FreeLibrary.
        //
        // The only caller is process teardown, which runs under the
        // loader lock; FreeLibrary from there is documented to
        // deadlock and the old loader did it from a static
        // destructor for no gain. Windows unmaps every module when
        // the process exits, so the "leak" lasts microseconds and
        // costs nothing.
        m_Modules.clear();
        m_Names.clear();
    }

    size_t Count() const { return m_Modules.size(); }

private:
    std::vector<HMODULE>     m_Modules;
    std::vector<std::string> m_Names;
    bool                     m_ShutdownCalled = false;
};

} // namespace kryotoo
