/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <engine/shared/network.h>
#include <engine/shared/config.h>
#include <engine/console.h>
#include <engine/masterserver.h>

#include <proxy/proxy/0.5/mastersrv.h>
#include <proxy/proxy/0.6/mastersrv.h>

#include "hacks_register.h"

CHacksRegister::CHacksRegister()
{
	m_pNetServer = 0;
	m_pMasterServer = 0;
	m_pConsole = 0;

	m_RegisterState = REGISTERSTATE_START;
	m_RegisterStateStart5 = 0;
	m_RegisterStateStart6 = 0;
	m_RegisterFirst = 1;
	m_RegisterCount = 0;

	mem_zero(m_aMasterserverInfo, sizeof(m_aMasterserverInfo));
	m_RegisterRegisteredServer5 = -1;
	m_RegisterRegisteredServer6 = -1;
}

void CHacksRegister::RegisterNewState(int State)
{
	m_RegisterState = State;
	int64 Now = time_get();
	m_RegisterStateStart5 = Now;
	m_RegisterStateStart6 = Now;
}

void CHacksRegister::RegisterSendFwcheckresponse(NETADDR *pAddr)
{
	CNetChunk Packet;
	Packet.m_ClientID = -1;
	Packet.m_Address = *pAddr;
	Packet.m_Flags = NETSENDFLAG_CONNLESS;
	// SERVERBROWSE_FWRESPONSE is the same for 0.5 & 0.6
	Packet.m_DataSize = sizeof(Protocol6::SERVERBROWSE_FWRESPONSE);
	Packet.m_pData = Protocol6::SERVERBROWSE_FWRESPONSE;
	m_pNetServer->Send(&Packet);
}

void CHacksRegister::RegisterSendHeartbeat6(NETADDR Addr)
{
	static unsigned char aData[sizeof(Protocol6::SERVERBROWSE_HEARTBEAT) + 2];
	unsigned short Port = g_Config.m_SvPort;
	CNetChunk Packet;

	mem_copy(aData, Protocol6::SERVERBROWSE_HEARTBEAT, sizeof(Protocol6::SERVERBROWSE_HEARTBEAT));

	Packet.m_ClientID = -1;
	Packet.m_Address = Addr;
	Packet.m_Flags = NETSENDFLAG_CONNLESS;
	Packet.m_DataSize = sizeof(Protocol6::SERVERBROWSE_HEARTBEAT) + 2;
	Packet.m_pData = &aData;

	// supply the set port that the master can use if it has problems
	if(g_Config.m_SvExternalPort)
		Port = g_Config.m_SvExternalPort;
	aData[sizeof(Protocol6::SERVERBROWSE_HEARTBEAT)] = Port >> 8;
	aData[sizeof(Protocol6::SERVERBROWSE_HEARTBEAT)+1] = Port&0xff;
	m_pNetServer->Send(&Packet);
}

void CHacksRegister::RegisterSendHeartbeat5(NETADDR Addr)
{
	static unsigned char aData[sizeof(Protocol5::SERVERBROWSE_HEARTBEAT) + 2];
	unsigned short Port = g_Config.m_SvPort;
	CNetChunk Packet;

	mem_copy(aData, Protocol5::SERVERBROWSE_HEARTBEAT, sizeof(Protocol5::SERVERBROWSE_HEARTBEAT));

	Packet.m_ClientID = -1;
	Packet.m_Address = Addr;
	Packet.m_Flags = NETSENDFLAG_CONNLESS;
	Packet.m_DataSize = sizeof(Protocol5::SERVERBROWSE_HEARTBEAT) + 2;
	Packet.m_pData = &aData;

	// supply the set port that the master can use if it has problems
	if(g_Config.m_SvExternalPort)
		Port = g_Config.m_SvExternalPort;
	aData[sizeof(Protocol5::SERVERBROWSE_HEARTBEAT)] = Port >> 8;
	aData[sizeof(Protocol5::SERVERBROWSE_HEARTBEAT)+1] = Port&0xff;
	m_pNetServer->Send(&Packet);
}

void CHacksRegister::RegisterSendCountRequest(NETADDR Addr)
{
	CNetChunk Packet;
	Packet.m_ClientID = -1;
	Packet.m_Address = Addr;
	Packet.m_Flags = NETSENDFLAG_CONNLESS;
	// SERVERBROWSE_GETCOUNT yields the same number if requested for 0.5 or 0.6
	Packet.m_DataSize = sizeof(Protocol6::SERVERBROWSE_GETCOUNT);
	Packet.m_pData = Protocol6::SERVERBROWSE_GETCOUNT;
	m_pNetServer->Send(&Packet);
}

void CHacksRegister::RegisterGotCount(CNetChunk *pChunk)
{
	unsigned char *pData = (unsigned char *)pChunk->m_pData;
	int Count = (pData[sizeof(Protocol6::SERVERBROWSE_COUNT)]<<8) | pData[sizeof(Protocol6::SERVERBROWSE_COUNT)+1];

	for(int i = 0; i < IMasterServer::MAX_MASTERSERVERS; i++)
	{
		if(net_addr_comp(&m_aMasterserverInfo[i].m_Addr, &pChunk->m_Address) == 0)
		{
			m_aMasterserverInfo[i].m_Count = Count;
			break;
		}
	}
}

void CHacksRegister::Init(CNetServer *pNetServer, IEngineMasterServer *pMasterServer, IConsole *pConsole)
{
	m_pNetServer = pNetServer;
	m_pMasterServer = pMasterServer;
	m_pConsole = pConsole;
}

void CHacksRegister::RegisterUpdate(int Nettype)
{
	int64 Now = time_get();
	int64 Freq = time_freq();

	if(!g_Config.m_SvRegister)
		return;

	m_pMasterServer->Update();

	if(m_RegisterState == REGISTERSTATE_START)
	{
		m_RegisterCount = 0;
		m_RegisterFirst = 1;
		RegisterNewState(REGISTERSTATE_UPDATE_ADDRS);
		m_pMasterServer->RefreshAddresses(Nettype);
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "register56", "refreshing ip addresses");
	}
	else if(m_RegisterState == REGISTERSTATE_UPDATE_ADDRS)
	{
		m_RegisterRegisteredServer5 = -1;
		m_RegisterRegisteredServer6 = -1;

		if(!m_pMasterServer->IsRefreshing())
		{
			int i;
			for(i = 0; i < IMasterServer::MAX_MASTERSERVERS; i++)
			{
				if(!m_pMasterServer->IsValid(i))
				{
					m_aMasterserverInfo[i].m_Valid = 0;
					m_aMasterserverInfo[i].m_Count = 0;
					continue;
				}

				NETADDR Addr = m_pMasterServer->GetAddr(i);
				m_aMasterserverInfo[i].m_Addr = Addr;
				m_aMasterserverInfo[i].m_Valid = 1;
				m_aMasterserverInfo[i].m_Count = -1;
				m_aMasterserverInfo[i].m_LastSend = 0;
			}

			m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "register56", "fetching server counts");
			RegisterNewState(REGISTERSTATE_QUERY_COUNT);
		}
	}
	else if(m_RegisterState == REGISTERSTATE_QUERY_COUNT)
	{
		int Left = 0;
		for(int i = 0; i < IMasterServer::MAX_MASTERSERVERS; i++)
		{
			if(!m_aMasterserverInfo[i].m_Valid)
				continue;

			if(m_aMasterserverInfo[i].m_Count == -1)
			{
				Left++;
				if(m_aMasterserverInfo[i].m_LastSend+Freq < Now)
				{
					m_aMasterserverInfo[i].m_LastSend = Now;
					RegisterSendCountRequest(m_aMasterserverInfo[i].m_Addr);
				}
			}
		}

		// check if we are done or timed out
		if(Left == 0 || Now > m_RegisterStateStart5+Freq*3 || Now > m_RegisterStateStart6+Freq*3)
		{
			// choose server
			int First = -1;
			int Second = -1;
			for(int i = 0; i < IMasterServer::MAX_MASTERSERVERS; i++)
			{
				if(!m_aMasterserverInfo[i].m_Valid || m_aMasterserverInfo[i].m_Count == -1)
					continue;

				int Count = m_aMasterserverInfo[i].m_Count;

				if(First == -1 || Count < m_aMasterserverInfo[First].m_Count)
				{
					Second = First;
					First = i;
				}
				else if(Second == -1 || Count < m_aMasterserverInfo[Second].m_Count)
				{
					Second = i;
				}
			}
			if(First == -1)
			{
				m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "register56", "WARNING: No master servers. Retrying in 60 seconds");
				RegisterNewState(REGISTERSTATE_ERROR);
			}
			else if(Second == -1)
			{
				m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "register56", "WARNING: Only one master server (need two). Retrying in 60 seconds");
				RegisterNewState(REGISTERSTATE_ERROR);
			}
			// server chosen
			else
			{
				m_RegisterRegisteredServer5 = Second;
				m_RegisterRegisteredServer6 = First;
				char aBuf[256];
				str_format(aBuf, sizeof(aBuf), "chose '%s' as master for 0.5, sending heartbeats", m_pMasterServer->GetName(m_RegisterRegisteredServer5));
				m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "register56", aBuf);
				str_format(aBuf, sizeof(aBuf), "chose '%s' as master for 0.6, sending heartbeats", m_pMasterServer->GetName(m_RegisterRegisteredServer6));
				m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "register56", aBuf);
				m_aMasterserverInfo[m_RegisterRegisteredServer5].m_LastSend = 0;
				m_aMasterserverInfo[m_RegisterRegisteredServer6].m_LastSend = 0;
				RegisterNewState(REGISTERSTATE_HEARTBEAT);
			}
		}
	}
	else if(m_RegisterState == REGISTERSTATE_HEARTBEAT)
	{
		// check if we should send heartbeat
		if(Now > m_aMasterserverInfo[m_RegisterRegisteredServer6].m_LastSend+Freq*15)
		{
			m_aMasterserverInfo[m_RegisterRegisteredServer6].m_LastSend = Now;
			// if this version isn't completed yet
			if(m_RegisterStateStart6 <= m_RegisterStateStart5)
				RegisterSendHeartbeat6(m_aMasterserverInfo[m_RegisterRegisteredServer6].m_Addr);
		}
		if(Now > m_aMasterserverInfo[m_RegisterRegisteredServer5].m_LastSend+Freq*15)
		{
			m_aMasterserverInfo[m_RegisterRegisteredServer5].m_LastSend = Now;
			// if this version isn't completed yet
			if(m_RegisterStateStart5 <= m_RegisterStateStart6)
				RegisterSendHeartbeat5(m_aMasterserverInfo[m_RegisterRegisteredServer5].m_Addr);
		}

		if(Now > m_RegisterStateStart5+Freq*60 || Now > m_RegisterStateStart6+Freq*60)
		{
			m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "register56", "WARNING: At least one master server is not responding, switching master");
			RegisterNewState(REGISTERSTATE_START);
		}
	}
	else if(m_RegisterState == REGISTERSTATE_REGISTERED)
	{
		if(m_RegisterFirst)
			m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "register56", "server registered");

		m_RegisterFirst = 0;

		// check if we should send new heartbeat again
		if(Now > m_RegisterStateStart5+Freq || Now > m_RegisterStateStart6+Freq)
		{
			if(m_RegisterCount == 120) // redo the whole process after 60 minutes to balance out the master servers
				RegisterNewState(REGISTERSTATE_START);
			else
			{
				m_RegisterCount++;
				RegisterNewState(REGISTERSTATE_HEARTBEAT);
			}
		}
	}
	else if(m_RegisterState == REGISTERSTATE_ERROR)
	{
		// check for restart
		if(Now > m_RegisterStateStart5+Freq*60 || Now > m_RegisterStateStart6+Freq*60)
			RegisterNewState(REGISTERSTATE_START);
	}
}

int CHacksRegister::RegisterProcessPacket(CNetChunk *pPacket)
{
	// check for masterserver address
	bool Valid = false;
	NETADDR Addr1 = pPacket->m_Address;
	Addr1.port = 0;
	for(int i = 0; i < IMasterServer::MAX_MASTERSERVERS; i++)
	{
		NETADDR Addr2 = m_aMasterserverInfo[i].m_Addr;
		Addr2.port = 0;
		if(net_addr_comp(&Addr1, &Addr2) == 0)
		{
			Valid = true;
			break;
		}
	}
	if(!Valid)
		return 0;

	// SERVERBROWSE_FWCHECK is the same for 0.5 & 0.6
	if(pPacket->m_DataSize == sizeof(Protocol6::SERVERBROWSE_FWCHECK) &&
		mem_comp(pPacket->m_pData, Protocol6::SERVERBROWSE_FWCHECK, sizeof(Protocol6::SERVERBROWSE_FWCHECK)) == 0)
	{
		RegisterSendFwcheckresponse(&pPacket->m_Address);
		return 1;
	}
	// SERVERBROWSE_FWOK is the same for 0.5 & 0.6
	else if(pPacket->m_DataSize == sizeof(Protocol6::SERVERBROWSE_FWOK) &&
		mem_comp(pPacket->m_pData, Protocol6::SERVERBROWSE_FWOK, sizeof(Protocol6::SERVERBROWSE_FWOK)) == 0)
	{
		NETADDR PacketAddr = pPacket->m_Address;                                       PacketAddr.port = 0;
		NETADDR Master6Addr = m_aMasterserverInfo[m_RegisterRegisteredServer6].m_Addr; Master6Addr.port = 0;
		NETADDR Master5Addr = m_aMasterserverInfo[m_RegisterRegisteredServer5].m_Addr; Master5Addr.port = 0;
		if(net_addr_comp(&PacketAddr, &Master6Addr) == 0)
		{
			if(m_RegisterStateStart6 < m_RegisterStateStart5)
			{
				if(m_RegisterFirst)
					m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "register56", "no firewall/nat problems detected");
				RegisterNewState(REGISTERSTATE_REGISTERED);
			}
			else if(m_RegisterStateStart6 == m_RegisterStateStart5)
				m_RegisterStateStart6 = time_get();
		}
		else if(net_addr_comp(&PacketAddr, &Master5Addr) == 0)
		{
			if(m_RegisterStateStart5 < m_RegisterStateStart6)
			{
				if(m_RegisterFirst)
					m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "register56", "no firewall/nat problems detected");
				RegisterNewState(REGISTERSTATE_REGISTERED);
			}
			else if(m_RegisterStateStart5 == m_RegisterStateStart6)
				m_RegisterStateStart5 = time_get();
		}
		return 1;
	}
	// SERVERBROWSE_FWERROR is the same for 0.5 & 0.6
	else if(pPacket->m_DataSize == sizeof(Protocol6::SERVERBROWSE_FWERROR) &&
		mem_comp(pPacket->m_pData, Protocol6::SERVERBROWSE_FWERROR, sizeof(Protocol6::SERVERBROWSE_FWERROR)) == 0)
	{
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "register56", "ERROR: the master server reports that clients can not connect to this server.");
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "ERROR: configure your firewall/nat to let through udp on port %d.", g_Config.m_SvPort);
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "register56", aBuf);
		RegisterNewState(REGISTERSTATE_ERROR);
		return 1;
	}
	// SERVERBROWSE_COUNT yields the same number if requested for 0.5 or 0.6
	else if(pPacket->m_DataSize == sizeof(Protocol6::SERVERBROWSE_COUNT)+2 &&
		mem_comp(pPacket->m_pData, Protocol6::SERVERBROWSE_COUNT, sizeof(Protocol6::SERVERBROWSE_COUNT)) == 0)
	{
		RegisterGotCount(pPacket);
		return 1;
	}

	return 0;
}

