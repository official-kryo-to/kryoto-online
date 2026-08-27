// Steam's callback and call-result plumbing.
//
// Every asynchronous answer a multiplayer game waits on arrives through
// here: matchmaking results, lobby create/join, the server list. The
// game registers a CCallback, the client pipe produces a CallbackMsg_t,
// and this maps one onto the other. Nothing that waits on Steam works
// without it.
//
// Plugins can register a patcher against an iCallback to mutate a
// message before the game sees it - see include/kryoto_plugin.h.

#pragma once

#include <map>

typedef bool (S_CALLTYPE* Fn_BGetCallback)(HSteamPipe hPipe, CallbackMsg_t* pMsg);
typedef void (S_CALLTYPE* Fn_FreeLastCallback)(HSteamPipe hPipe);
typedef bool (S_CALLTYPE* Fn_GetAPICallResult)(HSteamPipe hPipe, SteamAPICall_t hCall, void* pBuf, int cubBuf, int iExpected, bool* pbFailed);

class CCallbackDispatcher
{
public:
	Fn_BGetCallback m_pfnBGetCallback;
	Fn_FreeLastCallback m_pfnFreeLastCallback;
	Fn_GetAPICallResult m_pfnGetAPICallResult;
	HSteamUser m_CurrentUser;
	int m_ManualCbId;
	int m_ManualCbSize;
	bool m_bProcessing;
	std::multimap<int, CCallbackBase*> m_CallbackMap;
	std::map<SteamAPICall_t, CCallbackBase*> m_CallResultMap;
	std::map<SteamAPICall_t, BYTE*> m_BufferMap;

	CCallbackDispatcher();
	~CCallbackDispatcher();

	void Shutdown();
	void DispatchFrame(HSteamPipe hPipe, bool bServer);
	void DispatchFrameSafe(HSteamPipe hPipe, bool bServer);
	void ExecuteCallResult(HSteamPipe hPipe, SteamAPICall_t hCall, CCallbackBase* pCb);
	void Add(CCallbackBase* pCb, int iCallback);
	void AddCallResult(CCallbackBase* pCb, SteamAPICall_t hCall);
	void Remove(CCallbackBase* pCb);
	void RemoveCallResult(CCallbackBase* pCb, SteamAPICall_t hCall);
};

CCallbackDispatcher* GetDispatcher();
