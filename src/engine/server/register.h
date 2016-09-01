/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SERVER_REGISTER_H
#define ENGINE_SERVER_REGISTER_H

#include <engine/shared/http_request.h>
#include <engine/shared/network.h>

class CRegister
{
	enum
	{
		REGISTERSTATE_START=0,
		REGISTERSTATE_UPDATE_ADDRS,
		REGISTERSTATE_QUERY_COUNT,
		REGISTERSTATE_HEARTBEAT,
		REGISTERSTATE_FWCHECK,
		REGISTERSTATE_ERROR
	};

	struct CMasterserver
	{
		NETADDR m_Addr;
		int m_Count;
		int m_Valid;
		int64 m_Blacklisted;
		CHttpRequest m_HttpRequest;
	};

	class CNetServer *m_pNetServer;
	class IEngineMasterServer *m_pMasterServer;
	class IConsole *m_pConsole;

	int m_RegisterState;
	int64 m_RegisterStateStart;
	int m_RegisterFirst;
	int m_RegisterCount;
	int m_GotHeartbeatResponse;

	bool m_FwCheckAlternatePort;
	char m_aFwCheckToken[64];
	char m_aFwCheckResult[64];
	bool m_FwCheckResultValid;
	int m_NumFwChecks;
	int m_FwPort;

	CMasterserver m_aMasterservers[IMasterServer::MAX_MASTERSERVERS];
	int m_RegisterRegisteredServer;

	void RegisterNewState(int State);
	void RegisterSendHeartbeat(int i);
	void RegisterGotHeartbeatResponse(int i, char *pData);
	void RegisterSendCountRequest(int i);
	void RegisterGotCount(int i, char *pData);

	void BlacklistMaster(int i, int Seconds=300);

public:
	CRegister();
	void Init(class CNetServer *pNetServer, class IEngineMasterServer *pMasterServer, class IConsole *pConsole);
	void RegisterUpdate(int Nettype);
	int RegisterProcessPacket(struct CNetChunk *pPacket, TOKEN Token);
};

#endif
