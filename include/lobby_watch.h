// Which Steam lobby this game is in, for Kryoto Desktop's game invites.
//
// A friend invited from Kryoto Desktop should land in the same lobby, not
// just in the same game. The desktop cannot ask Steam which lobby a game is
// in, but this DLL sees every callback the game gets, so it writes it down:
//
//   %LOCALAPPDATA%\Kryoto\online\<pid>.json
//   { "pid": 1234, "lobby": "1097...", "host": "7656...", "ogAppId": "1966720", "updatedAt": 1759500000 }
//
// `lobby` is the lobby's 64-bit id, `host` the local player's own SteamID
// (the friend joins through them: steam://joinlobby/480/<lobby>/<host>),
// `updatedAt` Unix seconds, refreshed every 30 seconds while the game is in
// the lobby. The file goes when the game leaves the lobby, shuts Steam down
// or exits. Under Wine the path is inside the prefix and the pid is Wine's;
// the desktop allows for both (kryoto-desktop src-tauri/src/lobbies.rs).
//
// Where it learns things:
//   * LobbyEnter_t (a callback AND a call result) with a success response,
//     and LobbyCreated_t with k_EResultOK: in a lobby.
//   * SteamAPI_ISteamMatchmaking_LeaveLobby (the flat API Unity games use):
//     out of it.
//   * Every 5 seconds, GetNumLobbyMembers on the lobby: 0 means we are no
//     longer in it - which is how a game calling LeaveLobby through the C++
//     vtable (which never passes through this DLL) is noticed.
//
// It only ever writes this one local file, and only reads what the game
// already asked Steam for. Nothing is sent anywhere.

#pragma once

#include <Windows.h>
#include <stdio.h>
#include <time.h>

namespace KryotoLobby
{
	static SRWLOCK   s_Lock = SRWLOCK_INIT;
	static uint64    s_Lobby = 0;
	static ULONGLONG s_EnteredAt = 0;   // GetTickCount64 when s_Lobby was set
	static ULONGLONG s_WrittenAt = 0;
	static ULONGLONG s_CheckedAt = 0;

	static const ULONGLONG kCheckMs = 5000;      // membership check
	static const ULONGLONG kHeartbeatMs = 30000; // updatedAt refresh
	static const ULONGLONG kGraceMs = 10000;     // member list fills in after entering

	// %LOCALAPPDATA%\Kryoto\online\<pid>.json, creating the folders. False
	// when there is no LOCALAPPDATA (a service account), which just means no file.
	static bool FilePath(wchar_t* out, size_t cch, bool create)
	{
		wchar_t base[MAX_PATH] = { 0 };
		DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", base, MAX_PATH);
		if (n == 0 || n >= MAX_PATH)
			return false;

		wchar_t dir[MAX_PATH] = { 0 };
		if (_snwprintf_s(dir, MAX_PATH, _TRUNCATE, L"%s\\Kryoto", base) < 0)
			return false;
		if (create)
			CreateDirectoryW(dir, nullptr);
		if (_snwprintf_s(dir, MAX_PATH, _TRUNCATE, L"%s\\Kryoto\\online", base) < 0)
			return false;
		if (create)
			CreateDirectoryW(dir, nullptr);

		return _snwprintf_s(out, cch, _TRUNCATE, L"%s\\%lu.json", dir, GetCurrentProcessId()) >= 0;
	}

	static uint64 SelfSteamId()
	{
		ISteamUser* pUser = g_ClientCtx.SteamUser();
		return pUser ? pUser->GetSteamID().ConvertToUint64() : 0;
	}

	// Caller holds s_Lock.
	static void WriteLocked()
	{
		wchar_t path[MAX_PATH] = { 0 };
		wchar_t tmp[MAX_PATH] = { 0 };
		if (!FilePath(path, MAX_PATH, true))
			return;
		if (_snwprintf_s(tmp, MAX_PATH, _TRUNCATE, L"%s.tmp", path) < 0)
			return;

		// The host is blank when Steam has not told us who we are yet; the
		// desktop then joins by the lobby alone.
		char host[24] = { 0 };
		uint64 self = SelfSteamId();
		if (self)
			_snprintf_s(host, sizeof(host), _TRUNCATE, "%llu", (unsigned long long)self);

		char json[256] = { 0 };
		int len = _snprintf_s(json, sizeof(json), _TRUNCATE,
			"{\"pid\":%lu,\"lobby\":\"%llu\",\"host\":\"%s\",\"ogAppId\":\"%u\",\"updatedAt\":%lld}",
			GetCurrentProcessId(),
			(unsigned long long)s_Lobby,
			host,
			g_OriginalAppId,
			(long long)_time64(nullptr));
		if (len <= 0)
			return;

		// Written beside it and moved over it, so the desktop never reads half a file.
		HANDLE h = CreateFileW(tmp, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (h == INVALID_HANDLE_VALUE)
			return;
		DWORD wrote = 0;
		BOOL ok = WriteFile(h, json, (DWORD)len, &wrote, nullptr) && wrote == (DWORD)len;
		CloseHandle(h);
		if (!ok || !MoveFileExW(tmp, path, MOVEFILE_REPLACE_EXISTING))
		{
			DeleteFileW(tmp);
			return;
		}
		s_WrittenAt = GetTickCount64();
	}

	// Caller holds s_Lock.
	static void ClearLocked()
	{
		if (s_Lobby == 0)
			return;
		s_Lobby = 0;
		wchar_t path[MAX_PATH] = { 0 };
		if (FilePath(path, MAX_PATH, false))
			DeleteFileW(path);
		KRYOTOLOG("[KryotoOnline] Lobby: left\r\n");
	}

	static void Entered(uint64 lobby)
	{
		if (lobby == 0)
			return;
		AcquireSRWLockExclusive(&s_Lock);
		if (s_Lobby != lobby)
			KRYOTOLOG("[KryotoOnline] Lobby: entered %llu\r\n", (unsigned long long)lobby);
		s_Lobby = lobby;
		s_EnteredAt = GetTickCount64();
		WriteLocked();
		ReleaseSRWLockExclusive(&s_Lock);
	}

	// Any callback or call result the game receives. Only the two lobby ones
	// matter, and only with the size this build's SDK gives them: a struct of
	// another shape is not read.
	static void OnCallback(int iCallback, const void* p, int cb)
	{
		if (!p)
			return;
		if (iCallback == LobbyEnter_t::k_iCallback && cb == (int)sizeof(LobbyEnter_t))
		{
			const LobbyEnter_t* e = (const LobbyEnter_t*)p;
			if (e->m_EChatRoomEnterResponse == k_EChatRoomEnterResponseSuccess)
				Entered(e->m_ulSteamIDLobby);
		}
		else if (iCallback == LobbyCreated_t::k_iCallback && cb == (int)sizeof(LobbyCreated_t))
		{
			const LobbyCreated_t* c = (const LobbyCreated_t*)p;
			if (c->m_eResult == k_EResultOK)
				Entered(c->m_ulSteamIDLobby);
		}
	}

	static void OnLeave(uint64 lobby)
	{
		AcquireSRWLockExclusive(&s_Lock);
		if (lobby == s_Lobby)
			ClearLocked();
		ReleaseSRWLockExclusive(&s_Lock);
	}

	// Once per callback frame, on the game's own thread.
	static void Tick()
	{
		ULONGLONG now = GetTickCount64();
		if (now - s_CheckedAt < kCheckMs)
			return;
		s_CheckedAt = now;

		AcquireSRWLockExclusive(&s_Lock);
		if (s_Lobby != 0)
		{
			ISteamMatchmaking* pMM = g_ClientCtx.SteamMatchmaking();
			if (pMM && now - s_EnteredAt > kGraceMs && pMM->GetNumLobbyMembers(CSteamID(s_Lobby)) <= 0)
				ClearLocked();
			else if (now - s_WrittenAt >= kHeartbeatMs)
				WriteLocked();
		}
		ReleaseSRWLockExclusive(&s_Lock);
	}

	// SteamAPI_Shutdown and process exit.
	static void Shutdown()
	{
		AcquireSRWLockExclusive(&s_Lock);
		ClearLocked();
		ReleaseSRWLockExclusive(&s_Lock);
	}
}
