/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef PROXY_HACKS_HACKSREGISTER_H
#define PROXY_HACKS_HACKSREGISTER_H

#include <engine/masterserver.h>

class CHacksRegister
{
	enum
	{
		REGISTERSTATE_START=0,
		REGISTERSTATE_UPDATE_ADDRS,
		REGISTERSTATE_QUERY_COUNT,
		REGISTERSTATE_HEARTBEAT,
		REGISTERSTATE_REGISTERED,
		REGISTERSTATE_ERROR
	};

	struct CMasterserverInfo
	{
		NETADDR m_Addr;
		int m_Count;
		int m_Valid;
		int64 m_LastSend;
	};

	class CNetServer *m_pNetServer;
	class IEngineMasterServer *m_pMasterServer;
	class IConsole *m_pConsole;

	int m_RegisterState;
	int64 m_RegisterStateStart5;
	int64 m_RegisterStateStart6;
	int m_RegisterFirst;
	int m_RegisterCount;

	CMasterserverInfo m_aMasterserverInfo[IMasterServer::MAX_MASTERSERVERS];
	int m_RegisterRegisteredServer5;
	int m_RegisterRegisteredServer6;

	void RegisterNewState(int State);
	void RegisterSendFwcheckresponse(NETADDR *pAddr);
	void RegisterSendHeartbeat5(NETADDR Addr);
	void RegisterSendHeartbeat6(NETADDR Addr);
	void RegisterSendCountRequest(NETADDR Addr);
	void RegisterGotCount(struct CNetChunk *pChunk);

public:
	CHacksRegister();
	void Init(class CNetServer *pNetServer, class IEngineMasterServer *pMasterServer, class IConsole *pConsole);
	void RegisterUpdate(int Nettype);
	int RegisterProcessPacket(struct CNetChunk *pPacket);
};

#endif
