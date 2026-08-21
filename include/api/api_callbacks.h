extern bool s_bDispatcherInited;
CCallbackDispatcher* GetDispatcher();
S_API void S_CALLTYPE Steam_RegisterInterfaceFuncs(void* hModule);
S_API void S_CALLTYPE Steam_RunCallbacks(HSteamPipe hPipe, bool bServer);

static void DoFrame(HSteamPipe hPipe, bool bServer)
{
	if (bServer || !g_pSteamClient || hPipe != g_ClientPipe)
		return;

	if (!g_pUtilsForCallbacks)
	{
		g_pUtilsForCallbacks = (ISteamUtils*)g_pSteamClient->GetISteamGenericInterface(0, g_ClientPipe, STEAMUTILS_INTERFACE_VERSION);
		if (g_pUtilsForCallbacks)
		{
			g_pUtilsForCallbacks->GetAppID();
			g_pUtilsForCallbacks->RunFrame();
		}
	}
	else
	{
		g_pUtilsForCallbacks->RunFrame();
	}

	if (!g_pControllerForCallbacks)
		g_pControllerForCallbacks = (ISteamController*)g_pSteamClient->GetISteamGenericInterface(g_ClientUser, g_ClientPipe, STEAMCONTROLLER_INTERFACE_VERSION);

	if (!g_pInputForCallbacks)
		g_pInputForCallbacks = (ISteamInput*)g_pSteamClient->GetISteamGenericInterface(g_ClientUser, g_ClientPipe, STEAMINPUT_INTERFACE_VERSION);

	if (g_pInputForCallbacks)
		g_pInputForCallbacks->RunFrame();

	if (g_pControllerForCallbacks)
		g_pControllerForCallbacks->RunFrame();
}

S_API void S_CALLTYPE SteamAPI_ManualDispatch_Init()
{
	KRYOTOLOG("[KryotoOnline] SteamAPI_ManualDispatch_Init\r\n");

	ISteamClient* pClient = g_ServerClient;
	if (g_pSteamClient) pClient = g_pSteamClient;

	if (pClient)
	{
		if (g_DispatchMode != 2)
		{
			if (g_DispatchMode == 0 || g_DispatchMode == 3)
			{
				g_DispatchMode = 1;
				pClient->Set_SteamAPI_CCheckCallbackRegisteredInProcess(nullptr);
			}
			else
			{
				KryotoColor(FOREGROUND_RED | FOREGROUND_INTENSITY, "[KryotoOnline] ManualDispatch_Init already called\r\n");
			}
		}
		else
		{
			KryotoColor(FOREGROUND_RED | FOREGROUND_INTENSITY, "[KryotoOnline] ManualDispatch_Init: standard dispatch already selected\r\n");
		}
	}
	else
	{
		KryotoColor(FOREGROUND_RED | FOREGROUND_INTENSITY, "[KryotoOnline] ManualDispatch_Init: no Steam client initialized\r\n");
	}
}

S_API void S_CALLTYPE SteamAPI_ManualDispatch_RunFrame(HSteamPipe hPipe)
{
	if (g_DispatchMode == 3)
		SteamAPI_ManualDispatch_Init();

	if (g_DispatchMode == 1)
	{
		if (hPipe != 0)
		{
			if (hPipe == g_ClientPipe)
			{
				DoFrame(hPipe, false);
			}
			else if (hPipe != g_ServerPipe)
			{
				KryotoColor(FOREGROUND_RED | FOREGROUND_INTENSITY, "[KryotoOnline] ManualDispatch_RunFrame: wrong pipe\r\n");
			}
		}
		else
		{
			KryotoColor(FOREGROUND_RED | FOREGROUND_INTENSITY, "[KryotoOnline] ManualDispatch_RunFrame: pipe is zero\r\n");
		}
	}
	else
	{
		KryotoColor(FOREGROUND_RED | FOREGROUND_INTENSITY, "[KryotoOnline] ManualDispatch_RunFrame: init not called\r\n");
	}
}

S_API bool S_CALLTYPE SteamAPI_ManualDispatch_GetNextCallback(HSteamPipe hPipe, CallbackMsg_t* pMsg)
{
	if (g_DispatchMode == 3)
		SteamAPI_ManualDispatch_Init();

	if (g_DispatchMode == 1)
	{
		CCallbackDispatcher* pDisp = GetDispatcher();

		if (pDisp->m_pfnBGetCallback)
		{
			if (pMsg)
			{
				if (!pDisp->m_bProcessing)
				{
					pDisp->m_bProcessing = true;

					if (pDisp->m_pfnBGetCallback(hPipe, pMsg))
					{
						pDisp->m_ManualCbId = pMsg->m_iCallback;
						pDisp->m_ManualCbSize = pMsg->m_cubParam;
						return true;
					}

					pDisp->m_bProcessing = false;
				}
				else
				{
					KryotoColor(FOREGROUND_RED | FOREGROUND_INTENSITY, "[KryotoOnline] ManualDispatch_GetNextCallback: waiting for FreeLastCallback\r\n");
				}
			}
			else
			{
				KryotoColor(FOREGROUND_RED | FOREGROUND_INTENSITY, "[KryotoOnline] ManualDispatch_GetNextCallback: null msg ptr\r\n");
			}
		}
		else
		{
			KryotoColor(FOREGROUND_RED | FOREGROUND_INTENSITY, "[KryotoOnline] ManualDispatch_GetNextCallback: BGetCallback not available\r\n");
		}
	}
	else
	{
		KryotoColor(FOREGROUND_RED | FOREGROUND_INTENSITY, "[KryotoOnline] ManualDispatch_GetNextCallback: init not called\r\n");
	}

	return false;
}

S_API void S_CALLTYPE SteamAPI_ManualDispatch_FreeLastCallback(HSteamPipe hPipe)
{
	if (g_DispatchMode == 3)
		SteamAPI_ManualDispatch_Init();

	if (g_DispatchMode == 1)
	{
		CCallbackDispatcher* pDisp = GetDispatcher();

		if (pDisp->m_pfnFreeLastCallback)
		{
			if (pDisp->m_bProcessing)
			{
				pDisp->m_pfnFreeLastCallback(hPipe);
				pDisp->m_ManualCbId = 0;
				pDisp->m_ManualCbSize = 0;
				pDisp->m_bProcessing = false;
			}
			else
			{
				KryotoColor(FOREGROUND_RED | FOREGROUND_INTENSITY, "[KryotoOnline] ManualDispatch_FreeLastCallback: GetNextCallback not called\r\n");
			}
		}
		else
		{
			KryotoColor(FOREGROUND_RED | FOREGROUND_INTENSITY, "[KryotoOnline] ManualDispatch_FreeLastCallback: FreeLastCallback not available\r\n");
		}
	}
	else
	{
		KryotoColor(FOREGROUND_RED | FOREGROUND_INTENSITY, "[KryotoOnline] ManualDispatch_FreeLastCallback: init not called\r\n");
	}
}

S_API bool S_CALLTYPE SteamAPI_ManualDispatch_GetAPICallResult(HSteamPipe hPipe, SteamAPICall_t hCall, void* pBuf, int cubBuf, int iExpected, bool* pbFailed)
{
	if (g_DispatchMode == 3)
		SteamAPI_ManualDispatch_Init();

	if (g_DispatchMode == 1)
	{
		CCallbackDispatcher* pDisp = GetDispatcher();

		if (pDisp->m_pfnGetAPICallResult)
		{
			if (pBuf && cubBuf > 0 && pbFailed)
			{
				if (pDisp->m_bProcessing)
				{
					if (pDisp->m_ManualCbId == SteamAPICallCompleted_t::k_iCallback && pDisp->m_ManualCbSize == sizeof(SteamAPICallCompleted_t))
					{
						pDisp->m_ManualCbId = 0;
						pDisp->m_ManualCbSize = 0;
						return pDisp->m_pfnGetAPICallResult(hPipe, hCall, pBuf, cubBuf, iExpected, pbFailed);
					}
					else
					{
						KryotoColor(FOREGROUND_RED | FOREGROUND_INTENSITY, "[KryotoOnline] ManualDispatch_GetAPICallResult: not SteamAPICallCompleted\r\n");
					}
				}
				else
				{
					KryotoColor(FOREGROUND_RED | FOREGROUND_INTENSITY, "[KryotoOnline] ManualDispatch_GetAPICallResult: GetNextCallback not called\r\n");
				}
			}
			else
			{
				KryotoColor(FOREGROUND_RED | FOREGROUND_INTENSITY, "[KryotoOnline] ManualDispatch_GetAPICallResult: invalid args\r\n");
			}
		}
		else
		{
			KryotoColor(FOREGROUND_RED | FOREGROUND_INTENSITY, "[KryotoOnline] ManualDispatch_GetAPICallResult: GetAPICallResult not available\r\n");
		}
	}
	else
	{
		KryotoColor(FOREGROUND_RED | FOREGROUND_INTENSITY, "[KryotoOnline] ManualDispatch_GetAPICallResult: init not called\r\n");
	}

	return false;
}

S_API void S_CALLTYPE SteamAPI_RegisterCallResult(CCallbackBase* pCb, SteamAPICall_t hCall)
{
	KRYOTOLOG("[KryotoOnline] SteamAPI_RegisterCallResult\r\n");

	if (g_DispatchMode == 1)
	{
		KryotoColor(FOREGROUND_RED | FOREGROUND_INTENSITY, "[KryotoOnline] RegisterCallResult: manual dispatch active\r\n");
		return;
	}

	g_DispatchMode = 2;
	GetDispatcher()->AddCallResult(pCb, hCall);
}

S_API void S_CALLTYPE SteamAPI_RegisterCallback(CCallbackBase* pCb, int32 iCb)
{
	KRYOTOLOG("[KryotoOnline] SteamAPI_RegisterCallback\r\n");

	if (g_DispatchMode == 1)
	{
		KryotoColor(FOREGROUND_RED | FOREGROUND_INTENSITY, "[KryotoOnline] RegisterCallback: manual dispatch active\r\n");
		return;
	}

	g_DispatchMode = 2;
	GetDispatcher()->Add(pCb, iCb);
}

S_API void S_CALLTYPE SteamAPI_UnregisterCallResult(CCallbackBase* pCb, SteamAPICall_t hCall)
{
	KRYOTOLOG("[KryotoOnline] SteamAPI_UnregisterCallResult\r\n");

	if (g_DispatchMode == 1)
	{
		KryotoColor(FOREGROUND_RED | FOREGROUND_INTENSITY, "[KryotoOnline] UnregisterCallResult: manual dispatch active\r\n");
		return;
	}

	g_DispatchMode = 2;

	if (s_bDispatcherInited)
		GetDispatcher()->RemoveCallResult(pCb, hCall);
}

S_API void S_CALLTYPE SteamAPI_UnregisterCallback(CCallbackBase* pCb)
{
	KRYOTOLOG("[KryotoOnline] SteamAPI_UnregisterCallback\r\n");

	if (g_DispatchMode == 1)
	{
		KryotoColor(FOREGROUND_RED | FOREGROUND_INTENSITY, "[KryotoOnline] UnregisterCallback: manual dispatch active\r\n");
		return;
	}

	g_DispatchMode = 2;

	if (s_bDispatcherInited)
		GetDispatcher()->Remove(pCb);
}

S_API void S_CALLTYPE SteamAPI_RunCallbacks()
{
	KRYOTOLOG("[KryotoOnline] SteamAPI_RunCallbacks\r\n");

	if (g_DispatchMode == 1)
		return;

	g_DispatchMode = 2;

	if (g_ClientPipe == 0)
	{
		SteamAPI_ReleaseCurrentThreadMemory();
		return;
	}

	AcquireSRWLockExclusive(&g_CallbackLock);
	Steam_RunCallbacks(g_ClientPipe, false);
	ReleaseSRWLockExclusive(&g_CallbackLock);
}

S_API void S_CALLTYPE SteamAPI_SetTryCatchCallbacks(bool bEnable)
{
	KRYOTOLOG("[KryotoOnline] SteamAPI_SetTryCatchCallbacks\r\n");
	g_bTryCatch = bEnable;
}

S_API HSteamUser S_CALLTYPE Steam_GetHSteamUserCurrent()
{
	KRYOTOLOG("[KryotoOnline] Steam_GetHSteamUserCurrent\r\n");

	return ::Steam_GetHSteamUserCurrent();
}

S_API void S_CALLTYPE Steam_RegisterInterfaceFuncs(void* hModule)
{
	KRYOTOLOG("[KryotoOnline] Steam_RegisterInterfaceFuncs\r\n");

	if (!hModule)
	{
		KryotoColor(FOREGROUND_RED | FOREGROUND_INTENSITY, "[KryotoOnline] RegisterInterfaceFuncs: null module\r\n");
		return;
	}

	if (g_ClientModule && hModule != g_ClientModule)
	{
		KryotoColor(FOREGROUND_RED | FOREGROUND_INTENSITY, "[KryotoOnline] RegisterInterfaceFuncs: wrong client module\r\n");
		return;
	}

	if (!g_ClientModule && g_ServerModule && hModule != g_ServerModule)
	{
		KryotoColor(FOREGROUND_RED | FOREGROUND_INTENSITY, "[KryotoOnline] RegisterInterfaceFuncs: wrong server module\r\n");
		return;
	}

	if (!g_ClientModule && !g_ServerModule)
	{
		KryotoColor(FOREGROUND_RED | FOREGROUND_INTENSITY, "[KryotoOnline] RegisterInterfaceFuncs: no module loaded\r\n");
		return;
	}

	CCallbackDispatcher* pDisp = GetDispatcher();
	pDisp->m_pfnBGetCallback = (Fn_BGetCallback)GetProcAddress((HMODULE)hModule, "Steam_BGetCallback");
	pDisp->m_pfnFreeLastCallback = (Fn_FreeLastCallback)GetProcAddress((HMODULE)hModule, "Steam_FreeLastCallback");
	pDisp->m_pfnGetAPICallResult = (Fn_GetAPICallResult)GetProcAddress((HMODULE)hModule, "Steam_GetAPICallResult");
}

S_API void S_CALLTYPE Steam_RunCallbacks(HSteamPipe hPipe, bool bServer)
{
	KRYOTOLOG("[KryotoOnline] Steam_RunCallbacks\r\n");

	if (g_DispatchMode == 1)
		return;

	g_DispatchMode = 2;

	if (hPipe == 0)
		return;

	CCallbackDispatcher* pDisp = GetDispatcher();
	DoFrame(hPipe, bServer);

	if (!pDisp->m_bProcessing)
	{
		pDisp->m_bProcessing = true;

		if (g_bTryCatch)
			pDisp->DispatchFrameSafe(hPipe, bServer);
		else
			pDisp->DispatchFrame(hPipe, bServer);

		pDisp->m_bProcessing = false;
		pDisp->m_CurrentUser = 0;
	}
	else
	{
		KryotoColor(FOREGROUND_RED | FOREGROUND_INTENSITY, "[KryotoOnline] Steam_RunCallbacks: already processing\r\n");
	}
}
