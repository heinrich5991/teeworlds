/* (c) heinrich5991. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.              */

#include "serverbrowser.h"

#include <mastersrv/mastersrv.h>
#include <engine/shared/packer.h>
#include <stdlib.h>

CStatsServerBrowser::CStatsServerBrowser()
{
	NETADDR Addr;
	mem_zero(&Addr, sizeof(Addr));
	Addr.type = NETTYPE_ALL;
	m_Net.Open(Addr, 0);

	m_pServers = 0;
	m_pMasterServers = 0;

	ClearServers();
}

CStatsServerBrowser::~CStatsServerBrowser()
{
	m_Net.Close();
}

void CStatsServerBrowser::Refresh()
{
	m_Refreshing = 1;
	
	ClearServers();
}

void CStatsServerBrowser::ClearServers()
{
	CServerEntry *pCurrent = m_pServers;
	CServerEntry *pNext;
	while(pCurrent)
	{
		pNext = pCurrent->m_pNext;
		delete pCurrent;
		pCurrent = pNext;
	}
	m_pServers = 0;
	m_pServersLast = 0;
	m_NumServers = 0;
	m_NumReceivedServers = 0;

	CMasterServerEntry *pMasterServer = m_pMasterServers;
	while(pMasterServer)
	{
		pMasterServer->m_RequestTime = 0;
		pMasterServer->m_RequestTries = 0;
		pMasterServer->m_NumServers = 0;
		pMasterServer->m_NumGotServers = 0;
		pMasterServer->m_Done = 0;
		
		pMasterServer = pMasterServer->m_pNext;
	}
}

CStatsServerBrowser::CServerEntry *CStatsServerBrowser::Find(const NETADDR *pAddr)
{
	CServerEntry *pCurrent = m_pServers;
	while(pCurrent && net_addr_comp(pAddr, &pCurrent->m_Addr) != 0)
		pCurrent = pCurrent->m_pNext;
	return pCurrent;
}

void CStatsServerBrowser::AddMaster(const char *pHostname)
{
	CMasterServerEntry *pMasterServer = new CMasterServerEntry();
	mem_zero(pMasterServer, sizeof(*pMasterServer));
	str_copy(pMasterServer->m_aHostname, pHostname, sizeof(pMasterServer->m_aHostname));
	
	if(m_pMasterServers)
		m_pMasterServersLast->m_pNext = pMasterServer;
	else
		m_pMasterServers = pMasterServer;

	m_pMasterServersLast = pMasterServer;
}

void CStatsServerBrowser::CheckMasterDone(CMasterServerEntry *pMasterServer)
{
	if(pMasterServer->m_NumServers > 0 && abs(pMasterServer->m_NumServers - pMasterServer->m_NumGotServers) <= MAX_SERVERS_MISSING)
		pMasterServer->m_Done = 1;
}

int CStatsServerBrowser::Update(int MaxActions)
{
	int RemainingActions = (MaxActions != 0) ? MaxActions : MAX_ACTIONS;
	m_Net.Update();

	CNetChunk Packet;

	while(RemainingActions && m_Refreshing)
	{
		if(m_Net.Recv(&Packet))
			ProcessPacket(&Packet);
		else if(!UpdateMasters())
			RemainingActions = (!RequestNextServerInfo()) ? RemainingActions : 1;
		RemainingActions--;

		if(Done())
			m_Refreshing = 0;
	}
	
	return m_Refreshing;
}

void CStatsServerBrowser::ProcessPacket(const CNetChunk *pPacket)
{
	static const unsigned char SERVERBROWSE_INFO_LEGACY[] = { 255, 255, 255, 255, 'i', 'n', 'f', 'o' };
	char aAddressStr[NETADDR_MAXSTRSIZE];
	net_addr_str(&pPacket->m_Address, aAddressStr, sizeof(aAddressStr));

	if(pPacket->m_DataSize == (int)sizeof(SERVERBROWSE_COUNT) + 2 && mem_comp(pPacket->m_pData, SERVERBROWSE_COUNT, sizeof(SERVERBROWSE_COUNT)) == 0)
	{
		CMasterServerEntry *pMasterServer = FindMaster(&pPacket->m_Address);
		if(!pMasterServer)
		{
			dbg_msg("stats/browser", "warning: received server count from non-master, addr=%s", aAddressStr);
			return;
		}

		char *pCount = ((char *)pPacket->m_pData) + sizeof(SERVERBROWSE_COUNT);
		pMasterServer->m_NumServers = (((int)*pCount) << 8) + (int)*(pCount + 1);
		CheckMasterDone(pMasterServer);
	}
	else if(pPacket->m_DataSize >= (int)sizeof(SERVERBROWSE_LIST) && mem_comp(pPacket->m_pData, SERVERBROWSE_LIST, sizeof(SERVERBROWSE_LIST)) == 0)
	{
		CMasterServerEntry *pMasterServer = FindMaster(&pPacket->m_Address);
		int MasterServerIndex = GetMasterIndex(&pPacket->m_Address);
		if(!pMasterServer)
		{
			dbg_msg("stats/browser", "warning: received server list from non-master, addr=%s", aAddressStr);
			return;
		}

		int Size = pPacket->m_DataSize - sizeof(SERVERBROWSE_LIST);
		int Num = Size/sizeof(CMastersrvAddr);
		CMastersrvAddr *pAddrs = (CMastersrvAddr *)((char*)pPacket->m_pData+sizeof(SERVERBROWSE_LIST));
		for(int i = 0; i < Num; i++)
		{
			NETADDR Addr;

			static const unsigned char IPV4Mapping[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF };

			// copy address
			if(!mem_comp(IPV4Mapping, pAddrs[i].m_aIp, sizeof(IPV4Mapping)))
			{
				mem_zero(&Addr, sizeof(Addr));
				Addr.type = NETTYPE_IPV4;
				Addr.ip[0] = pAddrs[i].m_aIp[12];
				Addr.ip[1] = pAddrs[i].m_aIp[13];
				Addr.ip[2] = pAddrs[i].m_aIp[14];
				Addr.ip[3] = pAddrs[i].m_aIp[15];
			}
			else
			{
				Addr.type = NETTYPE_IPV6;
				mem_copy(Addr.ip, pAddrs[i].m_aIp, sizeof(Addr.ip));
			}
			Addr.port = (pAddrs[i].m_aPort[0]<<8) | pAddrs[i].m_aPort[1];

			AddServer(&Addr, INFOVERSION_2, MasterServerIndex);
		}
		pMasterServer->m_NumGotServers += Num;

		CheckMasterDone(pMasterServer);
	}
	else if(pPacket->m_DataSize >= (int)sizeof(SERVERBROWSE_LIST_LEGACY) && mem_comp(pPacket->m_pData, SERVERBROWSE_LIST_LEGACY, sizeof(SERVERBROWSE_LIST_LEGACY)) == 0)
	{
		CMasterServerEntry *pMasterServer = FindMaster(&pPacket->m_Address);
		int MasterServerIndex = GetMasterIndex(&pPacket->m_Address);
		if(!pMasterServer)
		{
			dbg_msg("stats/browser", "warning: received server list from non-master, addr=%s", aAddressStr);
			return;
		}

		struct CMasterServerAddressLegacy
		{
			unsigned char m_aIp[4];
			unsigned short m_Port;
		};

		int Size = pPacket->m_DataSize - sizeof(SERVERBROWSE_LIST);
		int Num = Size / sizeof(CMasterServerAddressLegacy);
		CMasterServerAddressLegacy *pAddrs = (CMasterServerAddressLegacy *)((char*)pPacket->m_pData+sizeof(SERVERBROWSE_LIST));
		for(int i = 0; i < Num; i++)
		{
			NETADDR Addr;

			// convert address
			mem_zero(&Addr, sizeof(Addr));
			Addr.type = NETTYPE_IPV4;
			Addr.ip[0] = pAddrs[i].m_aIp[0];
			Addr.ip[1] = pAddrs[i].m_aIp[1];
			Addr.ip[2] = pAddrs[i].m_aIp[2];
			Addr.ip[3] = pAddrs[i].m_aIp[3];
			Addr.port = pAddrs[i].m_Port;

			AddServer(&Addr, INFOVERSION_1, MasterServerIndex);
		}
		pMasterServer->m_NumGotServers += Num;
		
		CheckMasterDone(pMasterServer);
	}
	else if((pPacket->m_DataSize >= (int)sizeof(SERVERBROWSE_INFO) && mem_comp(pPacket->m_pData, SERVERBROWSE_INFO, sizeof(SERVERBROWSE_INFO)) == 0) || (pPacket->m_DataSize >= (int)sizeof(SERVERBROWSE_INFO_LEGACY) && mem_comp(pPacket->m_pData, SERVERBROWSE_INFO_LEGACY, sizeof(SERVERBROWSE_INFO_LEGACY)) == 0))
	{
		CServerEntry *pServer = Find(&pPacket->m_Address);
		if(!pServer || !pServer->m_RequestTime)
		{
			dbg_msg("stats/browser", "warning: received server info from a not requested server, addr=%s", aAddressStr);
			return;
		}

		if(pServer->m_GotInfo)
		{
			dbg_msg("stats/browser", "warning: already got info from this server, addr=%s", aAddressStr);
			return;
		}

		if(pServer->m_RequestTries > 1)
		{
			//dbg_msg("stats/browser", "got server info from server after retrying, addr=%s attempt=%d", aAddressStr, pServer->m_RequestTries);
		}

		int PacketInfoVersion = INFOVERSION_UNKNOWN;

		if(pPacket->m_DataSize >= (int)sizeof(SERVERBROWSE_INFO) && mem_comp(pPacket->m_pData, SERVERBROWSE_INFO, sizeof(SERVERBROWSE_INFO)) == 0)
			PacketInfoVersion = INFOVERSION_2;
		else
			PacketInfoVersion = INFOVERSION_1;

		if(pServer->m_InfoVersion != PacketInfoVersion)
		{
			dbg_msg("stats/browser", "warning: got wrong info version from server, addr=%s requested=%d got=%d", aAddressStr, pServer->m_InfoVersion, PacketInfoVersion);
			return;
		}

		CServerInfo Info = { 0 };

		if(!ReadInfo((char*)pPacket->m_pData + (int)sizeof(SERVERBROWSE_INFO), pPacket->m_DataSize - sizeof(SERVERBROWSE_INFO), &Info, PacketInfoVersion, aAddressStr, pServer->m_RequestToken))
		{
			pServer->m_Info = Info;
			pServer->m_GotInfo = 1;
			pServer->m_Done = 1;
			pServer->m_Info.m_Latency = time_get() - pServer->m_RequestTime;
			pServer->m_Info.m_MasterServer = pServer->m_MasterServer;
			m_NumReceivedServers++;
		}

	}
	else
	{
		dbg_msg("stats/browser/network", "warning: got unknown packet, addr=%s", aAddressStr);
	}
}

const char *CStatsServerBrowser::MasterServerHostname(int Index) const
{
	CMasterServerEntry *pMasterServer = m_pMasterServers;
	while(pMasterServer && Index)
	{
		pMasterServer = pMasterServer->m_pNext;
		Index--;
	}
	return (pMasterServer) ? pMasterServer->m_aHostname : 0;
}

CStatsServerBrowser::CMasterServerEntry *CStatsServerBrowser::FindMaster(const NETADDR *pAddr)
{
	CMasterServerEntry *pMasterServer = m_pMasterServers;
	while(pMasterServer && net_addr_comp(pAddr, &pMasterServer->m_Addr) != 0)
		pMasterServer = pMasterServer->m_pNext;
	return pMasterServer;
}

int CStatsServerBrowser::GetMasterIndex(const NETADDR *pAddr)
{
	int i = 0;
	CMasterServerEntry *pMasterServer = m_pMasterServers;
	while(pMasterServer && net_addr_comp(pAddr, &pMasterServer->m_Addr) != 0)
	{
		pMasterServer = pMasterServer->m_pNext;
		i++;
	}
	return (pMasterServer) ? i : -1;
}

void CStatsServerBrowser::AddServer(const NETADDR *pAddr, int InfoVersion, int Master)
{
	if(Find(pAddr))
	{
		char aBuf[NETADDR_MAXSTRSIZE];
		net_addr_str(pAddr, aBuf, sizeof(aBuf));
		//dbg_msg("stats/browser", "warning: got address twice, addr=%s", aBuf);
		return;
	}

	CServerEntry *pServerEntry = new CServerEntry();
	mem_zero(pServerEntry, sizeof(*pServerEntry));
	pServerEntry->m_Addr = *pAddr;
	pServerEntry->m_InfoVersion = InfoVersion;
	pServerEntry->m_MasterServer = Master;

	if(m_pServersLast)
	{
		m_pServersLast->m_pNext = pServerEntry;
		m_pServersLast = pServerEntry;
	}
	else
	{
		m_pServers = pServerEntry;
		m_pServersLast = pServerEntry;
	}
	m_NumServers++;
}

int CStatsServerBrowser::UpdateMasters()
{
	CNetChunk CountPacket;
	mem_zero(&CountPacket, sizeof(CountPacket));
	CountPacket.m_ClientID = -1;
	CountPacket.m_Flags = NETSENDFLAG_CONNLESS;
	CountPacket.m_pData = SERVERBROWSE_GETCOUNT;
	CountPacket.m_DataSize = sizeof(SERVERBROWSE_GETCOUNT);

	CNetChunk ListPacket;
	mem_zero(&ListPacket, sizeof(ListPacket));
	ListPacket.m_ClientID = -1;
	ListPacket.m_Flags = NETSENDFLAG_CONNLESS;
	ListPacket.m_pData = SERVERBROWSE_GETLIST;
	ListPacket.m_DataSize = sizeof(SERVERBROWSE_GETLIST);
	
	CNetChunk ListLegacyPacket;
	mem_zero(&ListLegacyPacket, sizeof(ListLegacyPacket));
	ListLegacyPacket.m_ClientID = -1;
	ListLegacyPacket.m_Flags = NETSENDFLAG_CONNLESS;
	ListLegacyPacket.m_pData = SERVERBROWSE_GETLIST_LEGACY;
	ListLegacyPacket.m_DataSize = sizeof(SERVERBROWSE_GETLIST_LEGACY);

	CMasterServerEntry *pMasterServer = m_pMasterServers;

	int Requested = 0;

	while(pMasterServer && !Requested)
	{
		if(pMasterServer->m_Done || (pMasterServer->m_RequestTime != 0 && time_get() - pMasterServer->m_RequestTime < SERVERINFO_RETRY_TIME * time_freq()))
		{
			pMasterServer = pMasterServer->m_pNext;
			continue;
		}

		if(pMasterServer->m_RequestTries >= MAX_TRIES)
			pMasterServer->m_Done = 1;
		
		if(pMasterServer->m_LastLookup == 0 || pMasterServer->m_RequestTries > 0)
		{
			net_host_lookup(pMasterServer->m_aHostname, &pMasterServer->m_Addr, NETTYPE_ALL);
			pMasterServer->m_Addr.port = 8300;
		}

		if(pMasterServer->m_NumServers == 0)
		{
			CountPacket.m_Address = pMasterServer->m_Addr;
			m_Net.Send(&CountPacket);
		
			pMasterServer->m_RequestTime = time_get();
			pMasterServer->m_NumServers = 0;
			Requested = 1;
		}
		
		if(pMasterServer->m_NumServers == 0 || abs(pMasterServer->m_NumGotServers - pMasterServer->m_NumServers) > MAX_SERVERS_MISSING)
		{
			ListPacket.m_Address = pMasterServer->m_Addr;
			ListLegacyPacket.m_Address = pMasterServer->m_Addr;
			m_Net.Send(&ListPacket);
			m_Net.Send(&ListLegacyPacket);
		
			pMasterServer->m_RequestTime = time_get();
			pMasterServer->m_NumGotServers = 0;
			Requested = 1;
		}

		if(!Requested)
			pMasterServer = pMasterServer->m_pNext;
	}
	
	if(Requested)
		pMasterServer->m_RequestTries++;

	return Requested;
}

int CStatsServerBrowser::Done() const
{
	const CMasterServerEntry *pMasterServer = m_pMasterServers;
	while(pMasterServer)
	{
		if(!pMasterServer->m_Done)
			return 0;
		pMasterServer = pMasterServer->m_pNext;
	}

	const CServerEntry *pServerEntry = m_pServers;
	while(pServerEntry)
	{
		if(!pServerEntry->m_Done)
			return 0;
		pServerEntry = pServerEntry->m_pNext;
	}

	return 1;
}

int CStatsServerBrowser::RequestNextServerInfo()
{
	CServerEntry *pServerEntry = m_pServers;
	while(pServerEntry && ((time_get() - pServerEntry->m_RequestTime < SERVERINFO_RETRY_TIME * time_freq() && pServerEntry->m_RequestTime != 0) || pServerEntry->m_Done))
		pServerEntry = pServerEntry->m_pNext;

	if(!pServerEntry)
		return 1;

	if(pServerEntry->m_RequestTries >= MAX_TRIES)
	{
		pServerEntry->m_Done = 1;
		return 0;
	}

	if(pServerEntry->m_InfoVersion == INFOVERSION_1)
	{
		static const unsigned char SERVERBROWSE_GETINFO_LEGACY[] = { 255, 255, 255, 255, 'g', 'i', 'e', 'f' };
		
		CNetChunk Packet;
		Packet.m_ClientID = -1;
		Packet.m_Address = pServerEntry->m_Addr;
		Packet.m_Flags = NETSENDFLAG_CONNLESS;
		Packet.m_pData = SERVERBROWSE_GETINFO_LEGACY;
		Packet.m_DataSize = sizeof(SERVERBROWSE_GETINFO_LEGACY);

		m_Net.Send(&Packet);
	}
	else if(pServerEntry->m_InfoVersion == INFOVERSION_2)
	{
		char aBuf[sizeof(SERVERBROWSE_GETINFO) + 1];
		mem_copy(aBuf, SERVERBROWSE_GETINFO, sizeof(SERVERBROWSE_GETINFO));
		pServerEntry->m_RequestToken = rand() % 256;
		aBuf[sizeof(SERVERBROWSE_GETINFO)] = pServerEntry->m_RequestToken;

		CNetChunk Packet;
		Packet.m_ClientID = -1;
		Packet.m_Address = pServerEntry->m_Addr;
		Packet.m_Flags = NETSENDFLAG_CONNLESS;
		Packet.m_pData = aBuf;
		Packet.m_DataSize = sizeof(aBuf);

		m_Net.Send(&Packet);
	}
	else
		pServerEntry->m_Done = 1;
	
	pServerEntry->m_RequestTime = time_get();
	pServerEntry->m_RequestTries++;

	return 0;
}

const NETADDR *CStatsServerBrowser::Server(int Index) const
{
	Index++;
	CServerEntry *pServer = m_pServers;
	while(pServer)
	{
		Index--;
		if(Index == 0)
			return &pServer->m_Addr;
		pServer = pServer->m_pNext;
	}
	return 0;
}

const NETADDR *CStatsServerBrowser::NotReceivedServer(int Index) const
{
	Index++;
	CServerEntry *pServer = m_pServers;
	while(pServer)
	{
		if(!pServer->m_GotInfo)
			Index--;
		if(Index == 0)
			return &pServer->m_Addr;
		pServer = pServer->m_pNext;
	}
	return 0;
}

const NETADDR *CStatsServerBrowser::ReceivedServer(int Index) const
{
	Index++;
	CServerEntry *pServer = m_pServers;
	while(pServer)
	{
		if(pServer->m_GotInfo)
			Index--;
		if(Index == 0)
			return &pServer->m_Addr;
		pServer = pServer->m_pNext;
	}
	return 0;
}

const CServerInfo *CStatsServerBrowser::ServerInfo(int Index) const
{
	Index++;
	CServerEntry *pServer = m_pServers;
	while(pServer)
	{
		if(pServer->m_GotInfo)
			Index--;
		if(Index == 0)
			return &pServer->m_Info;
		pServer = pServer->m_pNext;
	}
	return 0;
}

int CStatsServerBrowser::ReadInfo(const char *pData, int DataSize, CServerInfo *pInfo, int InfoVersion, const char *pAddressStr, int RequestToken)
{
	CServerInfo Info = { 0 };
	Info.m_Version = InfoVersion;
	
	CUnpacker Up;

	if(InfoVersion == INFOVERSION_1)
	{
		Up.Reset(pData, DataSize);
		str_copy(Info.m_aVersion, Up.GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES), sizeof(Info.m_aVersion));
		str_copy(Info.m_aName, Up.GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES), sizeof(Info.m_aName));
		str_copy(Info.m_aMap, Up.GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES), sizeof(Info.m_aMap));
		str_copy(Info.m_aGameType, Up.GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES), sizeof(Info.m_aGameType));

		if(Info.m_aGameType[1] == 0) // game type is one character long, 0.4 gametypes?
		{
			if(Info.m_aGameType[0] == '0')
				str_copy(Info.m_aGameType, "DM", sizeof(Info.m_aGameType));
			else if(Info.m_aGameType[0] == '1')
				str_copy(Info.m_aGameType, "TDM", sizeof(Info.m_aGameType));
			else if(Info.m_aGameType[0] == '2')
				str_copy(Info.m_aGameType, "CTF", sizeof(Info.m_aGameType));
		}
		Info.m_Flags = str_toint(Up.GetString());
		Info.m_Progression = str_toint(Up.GetString());
		Info.m_NumClients = str_toint(Up.GetString());
		Info.m_MaxClients = str_toint(Up.GetString());

		// don't add invalid info to the server browser list
		if(Info.m_NumClients < 0 || Info.m_NumClients > MAX_CLIENTS || Info.m_MaxClients < 0 || Info.m_MaxClients > MAX_CLIENTS)
			return 1;

		for(int i = 0; i < Info.m_NumClients; i++)
		{
			str_copy(Info.m_aClients[i].m_aName, Up.GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES), sizeof(Info.m_aClients[i].m_aName));
			Info.m_aClients[i].m_Score = str_toint(Up.GetString());
		}

		str_copy(Info.m_aAddress, pAddressStr, sizeof(Info.m_aAddress));

		if(Up.Error())
			return 1;

		*pInfo = Info;
		return 0;
	}
	else if(InfoVersion == INFOVERSION_2)
	{
		Up.Reset(pData, DataSize);

		int Token = str_toint(Up.GetString());
		if(Token != RequestToken)
		{
			dbg_msg("stats/browser", "warning: received server info with invalid token, addr=%s ours=%d theirs=%d", pAddressStr, RequestToken, Token);
			return 1;
		}

		str_copy(Info.m_aVersion, Up.GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES), sizeof(Info.m_aVersion));
		str_copy(Info.m_aName, Up.GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES), sizeof(Info.m_aName));
		str_copy(Info.m_aMap, Up.GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES), sizeof(Info.m_aMap));
		str_copy(Info.m_aGameType, Up.GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES), sizeof(Info.m_aGameType));
		Info.m_Flags = str_toint(Up.GetString());
		Info.m_NumPlayers = str_toint(Up.GetString());
		Info.m_MaxPlayers = str_toint(Up.GetString());
		Info.m_NumClients = str_toint(Up.GetString());
		Info.m_MaxClients = str_toint(Up.GetString());

		// don't add invalid info to the server browser list
		if(Info.m_NumClients < 0 || Info.m_NumClients > MAX_CLIENTS || Info.m_MaxClients < 0 || Info.m_MaxClients > MAX_CLIENTS ||
			Info.m_NumPlayers < 0 || Info.m_NumPlayers > Info.m_NumClients || Info.m_MaxPlayers < 0 || Info.m_MaxPlayers > Info.m_MaxClients)
			return 1;

		for(int i = 0; i < Info.m_NumClients; i++)
		{
			str_copy(Info.m_aClients[i].m_aName, Up.GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES), sizeof(Info.m_aClients[i].m_aName));
			str_copy(Info.m_aClients[i].m_aClan, Up.GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES), sizeof(Info.m_aClients[i].m_aClan));
			Info.m_aClients[i].m_Country = str_toint(Up.GetString());
			Info.m_aClients[i].m_Score = str_toint(Up.GetString());
			Info.m_aClients[i].m_Player = str_toint(Up.GetString()) != 0 ? 1 : 0;
		}

		str_copy(Info.m_aAddress, pAddressStr, sizeof(Info.m_aAddress));

		if(Up.Error())
			return 1;

		*pInfo = Info;
		return 0;
	}
	else
		return 1; // unknown info version
}

