// Two registry readers, used for exactly one thing: finding where Steam
// is installed so steamclient(64).dll can be loaded from it.
//
// The client records its own path under HKCU on install and puts it
// nowhere else - not on PATH, not anywhere a normal file search would
// reach - so reading the registry is the only way to locate it.
//
// Heuristic antivirus does flag registry reads from an injected DLL.
// There is no way around it short of not finding Steam at all, which
// would mean not working.
#pragma once

#include <Windows.h>

LSTATUS GetRegistryDWORD(const char* cszPath, const char* cszKey, DWORD& dwValue)
{
	HKEY hKeyNode = nullptr;
	DWORD dwType = REG_DWORD;
	DWORD dwSize = sizeof(DWORD);

	LSTATUS GetRegKey = ERROR_SUCCESS;

	GetRegKey = RegOpenKeyExA(HKEY_CURRENT_USER, cszPath, 0, KEY_READ, &hKeyNode);

	if (GetRegKey != ERROR_SUCCESS)
		return GetRegKey;

	GetRegKey = RegQueryValueExA(hKeyNode, cszKey, nullptr, &dwType, (LPBYTE)&dwValue, &dwSize);

	RegCloseKey(hKeyNode);

	return GetRegKey;
}

LSTATUS GetRegistryString(const char* cszPath, const char* cszKey, char* szString, unsigned int uMaxBuf)
{
	DWORD dwType = REG_SZ;
	DWORD dwSize = uMaxBuf;

	LSTATUS GetRegKey = RegGetValueA(HKEY_CURRENT_USER, cszPath, cszKey, RRF_RT_REG_SZ, &dwType, szString, &dwSize);

	return GetRegKey;
}
