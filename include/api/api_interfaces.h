S_API ISteamApps* S_CALLTYPE SteamApps()
{
    return g_bClientReady ? &s_AppsStub : nullptr;  // Always return our stub
}

S_API ISteamClient* S_CALLTYPE SteamClient()
{
	KRYOTOLOG("[KryotoOnline] SteamClient\r\n");
	return g_pSteamClientSafe;
}

S_API ISteamController* S_CALLTYPE SteamController()
{
	KRYOTOLOG("[KryotoOnline] SteamController\r\n");
	return g_bClientReady ? g_ClientCtx.SteamController() : nullptr;
}

S_API ISteamFriends* S_CALLTYPE SteamFriends()
{
	KRYOTOLOG("[KryotoOnline] SteamFriends\r\n");
	return g_bClientReady ? g_ClientCtx.SteamFriends() : nullptr;
}

S_API ISteamClient* S_CALLTYPE SteamGameServerSteamClient()
{
	KRYOTOLOG("[KryotoOnline] SteamGameServerSteamClient\r\n");
	return g_bServerReady ? g_ServerCtx.SteamClient() : nullptr;
}

S_API ISteamGameServer* S_CALLTYPE SteamGameServer()
{
	KRYOTOLOG("[KryotoOnline] SteamGameServer\r\n");
	return g_bServerReady ? g_ServerCtx.SteamGameServer() : nullptr;
}

S_API ISteamApps* S_CALLTYPE SteamGameServerApps()
{
	KRYOTOLOG("[KryotoOnline] SteamGameServerApps\r\n");
	return g_bServerReady ? g_ServerCtx.SteamApps() : nullptr;
}

S_API ISteamHTTP* S_CALLTYPE SteamGameServerHTTP()
{
	KRYOTOLOG("[KryotoOnline] SteamGameServerHTTP\r\n");
	return g_bServerReady ? g_ServerCtx.SteamHTTP() : nullptr;
}

S_API ISteamInventory* S_CALLTYPE SteamGameServerInventory()
{
	KRYOTOLOG("[KryotoOnline] SteamGameServerInventory\r\n");
	return g_bServerReady ? g_ServerCtx.SteamInventory() : nullptr;
}

S_API ISteamNetworking* S_CALLTYPE SteamGameServerNetworking()
{
	KRYOTOLOG("[KryotoOnline] SteamGameServerNetworking\r\n");
	return g_bServerReady ? g_ServerCtx.SteamGameServerNetworking() : nullptr;
}

S_API ISteamGameServerStats* S_CALLTYPE SteamGameServerStats()
{
	KRYOTOLOG("[KryotoOnline] SteamGameServerStats\r\n");
	return g_bServerReady ? g_ServerCtx.SteamGameServerStats() : nullptr;
}

S_API ISteamUGC* S_CALLTYPE SteamGameServerUGC()
{
	KRYOTOLOG("[KryotoOnline] SteamGameServerUGC\r\n");
	return g_bServerReady ? g_ServerCtx.SteamUGC() : nullptr;
}

S_API ISteamUtils* S_CALLTYPE SteamGameServerUtils()
{
	KRYOTOLOG("[KryotoOnline] SteamGameServerUtils\r\n");
	return g_bServerReady ? g_ServerCtx.SteamGameServerUtils() : nullptr;
}

S_API ISteamGameSearch* S_CALLTYPE SteamGameSearch()
{
	KRYOTOLOG("[KryotoOnline] SteamGameSearch\r\n");
	return g_bClientReady ? g_ClientCtx.SteamGameSearch() : nullptr;
}

S_API ISteamHTMLSurface* S_CALLTYPE SteamHTMLSurface()
{
	KRYOTOLOG("[KryotoOnline] SteamHTMLSurface\r\n");
	return g_bClientReady ? g_ClientCtx.SteamHTMLSurface() : nullptr;
}

S_API ISteamHTTP* S_CALLTYPE SteamHTTP()
{
	KRYOTOLOG("[KryotoOnline] SteamHTTP\r\n");
	return g_bClientReady ? g_ClientCtx.SteamHTTP() : nullptr;
}

S_API ISteamInput* S_CALLTYPE SteamInput()
{
	KRYOTOLOG("[KryotoOnline] SteamInput\r\n");
	return g_bClientReady ? g_ClientCtx.SteamInput() : nullptr;
}

S_API ISteamInventory* S_CALLTYPE SteamInventory()
{
	KRYOTOLOG("[KryotoOnline] SteamInventory\r\n");
	return g_bClientReady ? g_ClientCtx.SteamInventory() : nullptr;
}

S_API ISteamMatchmaking* S_CALLTYPE SteamMatchmaking()
{
	KRYOTOLOG("[KryotoOnline] SteamMatchmaking\r\n");
	return g_bClientReady ? g_ClientCtx.SteamMatchmaking() : nullptr;
}

S_API ISteamMatchmakingServers* S_CALLTYPE SteamMatchmakingServers()
{
	KRYOTOLOG("[KryotoOnline] SteamMatchmakingServers\r\n");
	return g_bClientReady ? g_ClientCtx.SteamMatchmakingServers() : nullptr;
}

S_API ISteamMusic* S_CALLTYPE SteamMusic()
{
	KRYOTOLOG("[KryotoOnline] SteamMusic\r\n");
	return g_bClientReady ? g_ClientCtx.SteamMusic() : nullptr;
}

S_API ISteamMusicRemote* S_CALLTYPE SteamMusicRemote()
{
	KRYOTOLOG("[KryotoOnline] SteamMusicRemote\r\n");
	return &s_MusicRemoteStub;
}

S_API ISteamNetworking* S_CALLTYPE SteamNetworking()
{
	KRYOTOLOG("[KryotoOnline] SteamNetworking\r\n");
	return g_bClientReady ? g_ClientCtx.SteamNetworking() : nullptr;
}

S_API ISteamParentalSettings* S_CALLTYPE SteamParentalSettings()
{
	KRYOTOLOG("[KryotoOnline] SteamParentalSettings\r\n");
	return g_bClientReady ? g_ClientCtx.SteamParentalSettings() : nullptr;
}

S_API ISteamParties* S_CALLTYPE SteamParties()
{
	KRYOTOLOG("[KryotoOnline] SteamParties\r\n");
	return g_bClientReady ? g_ClientCtx.SteamParties() : nullptr;
}

S_API ISteamRemotePlay* S_CALLTYPE SteamRemotePlay()
{
	KRYOTOLOG("[KryotoOnline] SteamRemotePlay\r\n");
	return g_bClientReady ? g_ClientCtx.SteamRemotePlay() : nullptr;
}

S_API ISteamRemoteStorage* S_CALLTYPE SteamRemoteStorage()
{
	KRYOTOLOG("[KryotoOnline] SteamRemoteStorage\r\n");
	return g_bClientReady ? g_ClientCtx.SteamRemoteStorage() : nullptr;
}

S_API ISteamScreenshots* S_CALLTYPE SteamScreenshots()
{
	KRYOTOLOG("[KryotoOnline] SteamScreenshots\r\n");
	return g_bClientReady ? g_ClientCtx.SteamScreenshots() : nullptr;
}

S_API ISteamUGC* S_CALLTYPE SteamUGC()
{
	KRYOTOLOG("[KryotoOnline] SteamUGC\r\n");
	return g_bClientReady ? g_ClientCtx.SteamUGC() : nullptr;
}

S_API ISteamUser* S_CALLTYPE SteamUser()
{
	KRYOTOLOG("[KryotoOnline] SteamUser\r\n");
	if (CSteamUserStub::IsEmulateTicketEnabled())
		return &s_UserStub;
	return g_bClientReady ? g_ClientCtx.SteamUser() : nullptr;
}

S_API ISteamUserStats* S_CALLTYPE SteamUserStats()
{
	KRYOTOLOG("[KryotoOnline] SteamUserStats\r\n");
	return g_bClientReady ? g_ClientCtx.SteamUserStats() : nullptr;
}

S_API ISteamUtils* S_CALLTYPE SteamUtils()
{
	KRYOTOLOG("[KryotoOnline] SteamUtils\r\n");
	return g_bClientReady ? g_ClientCtx.SteamUtils() : nullptr;
}

S_API ISteamVideo* S_CALLTYPE SteamVideo()
{
	KRYOTOLOG("[KryotoOnline] SteamVideo\r\n");
	return g_bClientReady ? g_ClientCtx.SteamVideo() : nullptr;
}
