/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>

#include <engine/config.h>
#include <engine/console.h>
#include <engine/kernel.h>
#include <engine/storage.h>

#include <engine/shared/config.h>
#include <engine/shared/netban.h>
#include <engine/shared/network.h>

#include "mastersrv.h"

int main(int argc, const char **argv) // ignore_convention
{
	dbg_logger_stdout();
	net_init();

	IKernel *pKernel = IKernel::Create();
	IStorage *pStorage = CreateStorage("Teeworlds", IStorage::STORAGETYPE_BASIC, argc, argv);
	IConfig *pConfig = CreateConfig();
	IConsole *pConsole = CreateConsole(CFGFLAG_MASTER);
	IMastersrv *pMastersrv = CreateMastersrv();

	bool RegisterFail = !pKernel->RegisterInterface(pStorage);
	RegisterFail |= !pKernel->RegisterInterface(pConsole);
	RegisterFail |= !pKernel->RegisterInterface(pConfig);
	RegisterFail |= !pKernel->RegisterInterface(pMastersrv);

	if(RegisterFail)
		return -1;

	pConfig->Init();
	if(argc > 1) // ignore_convention
		pConsole->ParseArguments(argc-1, &argv[1]); // ignore_convention

	int Result;

	if((Result = pMastersrv->Init()) != 0)
	{
		dbg_msg("mastersrv", "initialisation failed (%d)", Result);
		return Result;
	}

	Result = pMastersrv->Run();

	delete pMastersrv;
	delete pConsole;
	delete pConfig;
	delete pStorage;
	delete pKernel;

	return Result;
}

void IMastersrvSlave::NetaddrToMastersrv(CMastersrvAddr *pOut, const NETADDR *pIn)
{
	dbg_assert(pIn->type == NETTYPE_IPV6 || pIn->type == NETTYPE_IPV4, "nettype not supported");

	if(pIn->type == NETTYPE_IPV6)
	{
		mem_copy(pOut->m_aIp, pIn->ip, sizeof(pOut->m_aIp));
	}
	else if(pIn->type == NETTYPE_IPV4)
	{
		static const char aIPV4Mapping[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF };
		mem_copy(pOut->m_aIp, aIPV4Mapping, sizeof(aIPV4Mapping));
		mem_copy(pOut->m_aIp + 12, pIn->ip, 4);
	}

	pOut->m_aPort[0] = (pIn->port>>8)&0xff; // big endian
	pOut->m_aPort[1] = (pIn->port>>0)&0xff;
}

class CMastersrv : public IMastersrv
{
public:
	enum
	{
		MAX_SERVERS=1200,
		MAX_CHECKSERVERS=MAX_SERVERS,
		MAX_PACKETS=16,

		EXPIRE_TIME=90,
		BAN_REFRESH_TIME=300,
		PACKET_REFRESH_TIME=5,
	};
	CMastersrv();
	~CMastersrv();

	virtual int Init();
	virtual int Run();

	virtual void AddServer(const NETADDR *pAddr, void *pUserData, int Version);
	virtual void AddCheckserver(const NETADDR *pAddr, const NETADDR *pAltAddr, void *pUserData, int Version);
	virtual void SendList(const NETADDR *pAddr, void *pUserData, int Version);
	virtual int GetCount() const;

	virtual int Send(int Socket, const CNetChunk *pPacket, TOKEN PacketToken);
	virtual int SendRaw(int Socket, const NETADDR *pAddr, const void *pData, int DataSize);

	struct CRecvRawUserData
	{
		CMastersrv *m_pSelf;
		int m_Socket;
	};
	static int RecvRaw(const NETADDR *pAddr, const void *pData, int DataSize, void *pUserData)
	{
		CRecvRawUserData *pInfo = (CRecvRawUserData *)pUserData;
		return pInfo->m_pSelf->RecvRawImpl(pInfo->m_Socket, pAddr, pData, DataSize);
	}
	int RecvRawImpl(int Socket, const NETADDR *pAddr, const void *pData, int DataSize);
	CRecvRawUserData m_aRecvRawUserData[NUM_SOCKETS];

	void PurgeServers();
	void UpdateServers();
	void BuildPackets();

	void ReloadBans();

private:
	IConsole *m_pConsole;

	CNetBan m_NetBan;
	CNet m_aNets[NUM_SOCKETS];

	int64 m_BanRefreshTime;
	int64 m_PacketRefreshTime;

	struct CServerEntry
	{
		NETADDR m_Address;
		void *m_pSlaveUserData;

		int m_Version;
		int64 m_Expire;
	};

	struct CCheckServer
	{
		NETADDR m_Address;
		NETADDR m_AltAddress;
		void *m_pSlaveUserData;

		int m_TryCount;
		int64 m_TryTime;

		int m_Version;
	};

	CServerEntry m_aServers[MAX_SERVERS];
	int m_NumServers;
	CCheckServer m_aCheckServers[MAX_CHECKSERVERS];
	int m_NumCheckServers;

	struct CMastersrvSlave
	{
		struct CPacket
		{
			char m_aData[NET_MAX_PAYLOAD];
			int m_Size;
		} m_aPackets[MAX_PACKETS];
		int m_NumPackets;

		IMastersrvSlave *m_pSlave;
	};

	CMastersrvSlave m_aSlaves[NUM_MASTERSRV];
};

IMastersrv *CreateMastersrv()
{
	return new CMastersrv();
}

CMastersrv::CMastersrv()
{
	for(int s = 0; s < NUM_MASTERSRV; s++)
		m_aSlaves[s].m_pSlave = 0;
}

CMastersrv::~CMastersrv()
{
	for(int s = 0; s < NUM_MASTERSRV; s++)
		if(m_aSlaves[s].m_pSlave)
			delete m_aSlaves[s].m_pSlave;
}

int CMastersrv::Init()
{
	m_pConsole = Kernel()->RequestInterface<IConsole>();

	m_NetBan.Init(m_pConsole, Kernel()->RequestInterface<IStorage>());

	ReloadBans();
	m_BanRefreshTime = time_get() + time_freq() * BAN_REFRESH_TIME;

	int Services = 0;
	if(str_find(g_Config.m_MsServices, "5"))
		Services |= 1<<MASTERSRV_0_5;
	if(str_find(g_Config.m_MsServices, "6"))
		Services |= 1<<MASTERSRV_0_6;
	if(str_find(g_Config.m_MsServices, "7"))
		Services |= 1<<MASTERSRV_0_7;
	if(str_find(g_Config.m_MsServices, "v"))
		Services |= 1<<MASTERSRV_VER;

	for(int i = 0; i < NUM_SOCKETS; i++)
	{
		m_aRecvRawUserData[i].m_pSelf = this;
		m_aRecvRawUserData[i].m_Socket = i;
	}

	NETADDR BindAddr;
	if(g_Config.m_Bindaddr[0] && net_host_lookup(g_Config.m_Bindaddr, &BindAddr, NETTYPE_ALL) == 0)
	{
		// got bindaddr
		BindAddr.type = NETTYPE_ALL;
	}
	else
	{
		mem_zero(&BindAddr, sizeof(BindAddr));
		BindAddr.type = NETTYPE_ALL;
	}

	if(Services&((1<<MASTERSRV_0_5)|(1<<MASTERSRV_0_6)|(1<<MASTERSRV_0_7)))
	{
		BindAddr.port = MASTERSERVER_PORT;
		if(!m_aNets[SOCKET_OP].Open(&BindAddr, 0, 0, NETFLAG_ALLOWSTATELESS))
		{
			dbg_msg("mastersrv", "couldn't start network (op)");
			return -1;
		}
		BindAddr.port = MASTERSERVER_CHECKER_PORT;
		if(!m_aNets[SOCKET_CHECKER].Open(&BindAddr, 0, 0, NETFLAG_ALLOWSTATELESS))
		{
			dbg_msg("mastersrv", "couldn't start network (checker)");
			return -1;
		}

		m_aNets[SOCKET_OP].SetRecvRawCallback(RecvRaw, &m_aRecvRawUserData[SOCKET_OP]);
		m_aNets[SOCKET_CHECKER].SetRecvRawCallback(RecvRaw, &m_aRecvRawUserData[SOCKET_CHECKER]);
	}
	if(Services&(1<<MASTERSRV_VER))
	{
		BindAddr.port = VERSIONSERVER_PORT;
		if(!m_aNets[SOCKET_VERSION].Open(&BindAddr, 0, 0, NETFLAG_ALLOWSTATELESS))
		{
			dbg_msg("mastersrv", "couldn't start network (version)");
			return -1;
		}

		m_aNets[SOCKET_VERSION].SetRecvRawCallback(RecvRaw, &m_aRecvRawUserData[SOCKET_VERSION]);
	}

	// process pending commands
	m_pConsole->StoreCommands(false);

	for(int s = 0; s < NUM_MASTERSRV; s++)
		m_aSlaves[s].m_NumPackets = 0;

	if(Services&(1<<MASTERSRV_0_5))
		m_aSlaves[MASTERSRV_0_5].m_pSlave = CreateSlave_0_5(this);
	if(Services&(1<<MASTERSRV_0_6))
		m_aSlaves[MASTERSRV_0_6].m_pSlave = CreateSlave_0_6(this);
	if(Services&(1<<MASTERSRV_0_7))
		m_aSlaves[MASTERSRV_0_7].m_pSlave = CreateSlave_0_7(this);
	if(Services&(1<<MASTERSRV_VER))
		m_aSlaves[MASTERSRV_VER].m_pSlave = CreateSlave_Ver(this);

	return 0;
}

int CMastersrv::Run()
{
	dbg_msg("mastersrv", "started");

	while(1)
	{
		for(int i = 0; i < NUM_SOCKETS; i++)
			m_aNets[i].Update();

		for(int i = 0; i < NUM_SOCKETS; i++)
		{
			// process packets
			CNetChunk Packet;
			TOKEN Token;
			while(m_aNets[i].Recv(&Packet, &Token))
			{
				// check if the server is banned
				if(m_NetBan.IsBanned(&Packet.m_Address, 0, 0))
					continue;

				for(int s = 0; s < NUM_MASTERSRV; s++)
					if(m_aSlaves[s].m_pSlave)
					{
						if(m_aSlaves[s].m_pSlave->ProcessMessage(i, &Packet, Token) != 0)
							break;
					}
			}
		}

		int64 Now = time_get();
		int64 Freq = time_freq();
		if(m_BanRefreshTime < Now)
		{
			m_BanRefreshTime = Now + Freq * BAN_REFRESH_TIME;
			ReloadBans();
		}

		if(m_PacketRefreshTime < Now)
		{
			m_PacketRefreshTime = Now + Freq * PACKET_REFRESH_TIME;

			PurgeServers();
			UpdateServers();
			BuildPackets();
		}

		// be nice to the CPU
		thread_sleep(1);
	}

	return 0;
}

void CMastersrv::UpdateServers()
{
	int64 Now = time_get();
	int64 Freq = time_freq();
	for(int i = 0; i < m_NumCheckServers; i++)
	{
		CCheckServer *pCheck = &m_aCheckServers[i];

		if(pCheck->m_TryTime + Freq < Now)
		{
			IMastersrvSlave *pSlave = m_aSlaves[pCheck->m_Version].m_pSlave;
			dbg_assert(pSlave != 0, "attempting to access uninitalised slave");

			if(pCheck->m_TryCount == 10)
			{
				char aAddrStr[NETADDR_MAXSTRSIZE];
				net_addr_str(&pCheck->m_Address, aAddrStr, sizeof(aAddrStr), true);
				char aAltAddrStr[NETADDR_MAXSTRSIZE];
				net_addr_str(&pCheck->m_AltAddress, aAltAddrStr, sizeof(aAltAddrStr), true);
				dbg_msg("mastersrv", "check failed: %s (%s)", aAddrStr, aAltAddrStr);

				// FAIL!!
				pSlave->SendError(&pCheck->m_Address, pCheck->m_pSlaveUserData);
				*pCheck = m_aCheckServers[m_NumCheckServers-1];
				m_NumCheckServers--;
				i--;
			}
			else
			{
				pCheck->m_TryCount++;
				pCheck->m_TryTime = Now;

				if(pCheck->m_TryCount&1)
					pSlave->SendCheck(&pCheck->m_Address, pCheck->m_pSlaveUserData);
				else
					pSlave->SendCheck(&pCheck->m_AltAddress, pCheck->m_pSlaveUserData);
			}
		}
	}
}

void CMastersrv::BuildPackets()
{
	bool aPreparePacket[NUM_MASTERSRV];
	int BytesWritten;

	for(int s = 0; s < NUM_MASTERSRV; s++)
	{
		m_aSlaves[s].m_NumPackets = 0;
		aPreparePacket[s] = true;
		for(int i = 0; i < MAX_PACKETS; i++)
			m_aSlaves[s].m_aPackets[i].m_Size = 0;
	}

	for(int i = 0; i < m_NumServers; i++)
	{
		CServerEntry *pServer = &m_aServers[i];
		CMastersrvSlave *pSlaveData = &m_aSlaves[pServer->m_Version];
		IMastersrvSlave *pSlave = pSlaveData->m_pSlave;
		CMastersrvSlave::CPacket *pPacket = &pSlaveData->m_aPackets[pSlaveData->m_NumPackets - 1];
		dbg_assert(pSlave != 0, "attempting to access uninitalised slave");

		if(aPreparePacket[pServer->m_Version])
		{
			if(pSlaveData->m_NumPackets != 0)
			{
				BytesWritten = pSlave->BuildPacketFinalize(&pPacket->m_aData[pPacket->m_Size], NET_MAX_PAYLOAD - pPacket->m_Size);
				dbg_assert(BytesWritten >= 0, "build packet finalisation failed");
				pPacket->m_Size += BytesWritten;
			}

			pPacket = &pSlaveData->m_aPackets[pSlaveData->m_NumPackets];
			pSlaveData->m_NumPackets++;

			BytesWritten = pSlave->BuildPacketStart(&pPacket->m_aData[0], NET_MAX_PAYLOAD);

			dbg_assert(BytesWritten >= 0, "build packet initialisation failed");
			pPacket->m_Size += BytesWritten;

			aPreparePacket[pServer->m_Version] = false;
		}

		BytesWritten = pSlave->BuildPacketAdd(&pPacket->m_aData[pPacket->m_Size], NET_MAX_PAYLOAD - pPacket->m_Size,
			&pServer->m_Address, pServer->m_pSlaveUserData);
		if(BytesWritten < 0)
		{
			aPreparePacket[pServer->m_Version] = true;
			i--;
			continue;
		}
		pPacket->m_Size += BytesWritten;
	}

	for(int s = 0; s < NUM_MASTERSRV; s++)
	{
		CMastersrvSlave *pSlaveData = &m_aSlaves[s];
		IMastersrvSlave *pSlave = pSlaveData->m_pSlave;

		if(m_aSlaves[s].m_NumPackets > 0)
		{
			dbg_assert(pSlave != 0, "attempting to finalise packet for non-existant slave");

			CMastersrvSlave::CPacket *pPacket = &pSlaveData->m_aPackets[pSlaveData->m_NumPackets - 1];
			BytesWritten = pSlave->BuildPacketFinalize(&pPacket->m_aData[pPacket->m_Size], NET_MAX_PAYLOAD - pPacket->m_Size);
			dbg_assert(BytesWritten >= 0, "final build packet finalisation failed");
			pPacket->m_Size += BytesWritten;
		}
	}

/*	CServerEntry *pCurrent = &m_aServers[0];
	int ServersLeft = m_NumServers;
	m_NumPackets = 0;
	m_NumPacketsLegacy = 0;
	int PacketIndex = 0;
	int PacketIndexLegacy = 0;
	while(ServersLeft-- && (m_NumPackets + m_NumPacketsLegacy) < MAX_PACKETS)
	{
		if(pCurrent->m_Type == SERVERTYPE_NORMAL)
		{
			if(PacketIndex % MAX_SERVERS_PER_PACKET == 0)
			{
				PacketIndex = 0;
				m_NumPackets++;
			}

			// copy header
			mem_copy(m_aPackets[m_NumPackets-1].m_Data.m_aHeader, SERVERBROWSE_LIST, sizeof(SERVERBROWSE_LIST));

			// copy server addresses
			if(pCurrent->m_Address.type == NETTYPE_IPV6)
			{
				mem_copy(m_aPackets[m_NumPackets-1].m_Data.m_aServers[PacketIndex].m_aIp, pCurrent->m_Address.ip,
					sizeof(m_aPackets[m_NumPackets-1].m_Data.m_aServers[PacketIndex].m_aIp));
			}
			else
			{
				static char IPV4Mapping[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF };

				mem_copy(m_aPackets[m_NumPackets-1].m_Data.m_aServers[PacketIndex].m_aIp, IPV4Mapping, sizeof(IPV4Mapping));
				m_aPackets[m_NumPackets-1].m_Data.m_aServers[PacketIndex].m_aIp[12] = pCurrent->m_Address.ip[0];
				m_aPackets[m_NumPackets-1].m_Data.m_aServers[PacketIndex].m_aIp[13] = pCurrent->m_Address.ip[1];
				m_aPackets[m_NumPackets-1].m_Data.m_aServers[PacketIndex].m_aIp[14] = pCurrent->m_Address.ip[2];
				m_aPackets[m_NumPackets-1].m_Data.m_aServers[PacketIndex].m_aIp[15] = pCurrent->m_Address.ip[3];
			}

			m_aPackets[m_NumPackets-1].m_Data.m_aServers[PacketIndex].m_aPort[0] = (pCurrent->m_Address.port>>8)&0xff;
			m_aPackets[m_NumPackets-1].m_Data.m_aServers[PacketIndex].m_aPort[1] = pCurrent->m_Address.port&0xff;

			PacketIndex++;

			m_aPackets[m_NumPackets-1].m_Size = sizeof(SERVERBROWSE_LIST) + sizeof(CMastersrvAddr)*PacketIndex;

			pCurrent++;
		}
		else if(pCurrent->m_Type == SERVERTYPE_LEGACY)
		{
			if(PacketIndexLegacy % MAX_SERVERS_PER_PACKET == 0)
			{
				PacketIndexLegacy = 0;
				m_NumPacketsLegacy++;
			}

			// copy header
			mem_copy(m_aPacketsLegacy[m_NumPacketsLegacy-1].m_Data.m_aHeader, SERVERBROWSE_LIST_LEGACY, sizeof(SERVERBROWSE_LIST_LEGACY));

			// copy server addresses
			mem_copy(m_aPacketsLegacy[m_NumPacketsLegacy-1].m_Data.m_aServers[PacketIndexLegacy].m_aIp, pCurrent->m_Address.ip,
				sizeof(m_aPacketsLegacy[m_NumPacketsLegacy-1].m_Data.m_aServers[PacketIndexLegacy].m_aIp));
			// 0.5 has the port in little endian on the network
			m_aPacketsLegacy[m_NumPacketsLegacy-1].m_Data.m_aServers[PacketIndexLegacy].m_aPort[0] = pCurrent->m_Address.port&0xff;
			m_aPacketsLegacy[m_NumPacketsLegacy-1].m_Data.m_aServers[PacketIndexLegacy].m_aPort[1] = (pCurrent->m_Address.port>>8)&0xff; 
			PacketIndexLegacy++;

			m_aPacketsLegacy[m_NumPacketsLegacy-1].m_Size = sizeof(SERVERBROWSE_LIST_LEGACY) + sizeof(CMastersrvAddrLegacy)*PacketIndexLegacy;

			pCurrent++;
		}
		else
		{
			*pCurrent = m_aServers[m_NumServers-1];
			m_NumServers--;
			dbg_msg("mastersrv", "error: server of invalid type, dropping it");
		}
	}
*/
}

int CMastersrv::Send(int Socket, const CNetChunk *pPacket, TOKEN PacketToken)
{
	dbg_assert(Socket >= 0 && Socket < NUM_SOCKETS, "attempting to send via non-existant socket");

	m_aNets[Socket].Send((CNetChunk *)pPacket, PacketToken);

	return 0;
}

int CMastersrv::SendRaw(int Socket, const NETADDR *pAddr, const void *pData, int DataSize)
{
	dbg_assert(Socket >= 0 && Socket < NUM_SOCKETS, "attempting to send via non-existant socket");

	m_aNets[Socket].SendRaw(pAddr, pData, DataSize);

	return 0;
}

int CMastersrv::RecvRawImpl(int Socket, const NETADDR *pAddr, const void *pData, int DataSize)
{
	// check if the server is banned
	if(m_NetBan.IsBanned(pAddr, 0, 0))
		return 0;

	for(int s = 0; s < NUM_MASTERSRV; s++)
		if(m_aSlaves[s].m_pSlave)
		{
			if(m_aSlaves[s].m_pSlave->ProcessMessageRaw(Socket, pAddr, pData, DataSize) != 0)
				return 1;
		}
	return 0;
}


void CMastersrv::AddServer(const NETADDR *pAddr, void *pUserData, int Version)
{
	dbg_assert(Version >= 0 && Version < NUM_MASTERSRV, "version out of range");

	bool Found = false;
	for(int i = 0; i < m_NumCheckServers && !Found; i++)
	{
		if((net_addr_comp(&m_aCheckServers[i].m_Address, pAddr) == 0
			|| net_addr_comp(&m_aCheckServers[i].m_AltAddress, pAddr) == 0)
			&& m_aCheckServers[i].m_Version == Version)

		{
			m_NumCheckServers--;
			m_aCheckServers[i] = m_aCheckServers[m_NumCheckServers];
			Found = true;
		}
	}

	if(!Found) // only allow this for servers which are actually being checked
		return;

	char aAddrStr[NETADDR_MAXSTRSIZE];
	net_addr_str(pAddr, aAddrStr, sizeof(aAddrStr), true);

	// see if server already exists in list
	for(int i = 0; i < m_NumServers; i++)
	{
		if(net_addr_comp(&m_aServers[i].m_Address, pAddr) == 0 && m_aServers[i].m_Version == Version)
		{
			dbg_msg("mastersrv", "updated: %s", aAddrStr);
			m_aServers[i].m_pSlaveUserData = pUserData;
			m_aServers[i].m_Expire = time_get() + time_freq() * EXPIRE_TIME;
			IMastersrvSlave *pSlave = m_aSlaves[Version].m_pSlave;
			dbg_assert(pSlave != 0, "attempting to access uninitalised slave");
			pSlave->SendOk(pAddr, pUserData);
			return;
		}
	}

	// add server
	if(m_NumServers == MAX_SERVERS)
	{
		dbg_msg("mastersrv", "error: mastersrv is full: %s", aAddrStr);
		return;
	}

	dbg_msg("mastersrv", "added: %s", aAddrStr);
	m_aServers[m_NumServers].m_Address = *pAddr;
	m_aServers[m_NumServers].m_pSlaveUserData = pUserData;
	m_aServers[m_NumServers].m_Expire = time_get() + time_freq() * EXPIRE_TIME;
	m_aServers[m_NumServers].m_Version = Version;
	m_NumServers++;

	IMastersrvSlave *pSlave = m_aSlaves[Version].m_pSlave;
	dbg_assert(pSlave != 0, "attempting to access uninitalised slave");
	pSlave->SendOk(pAddr, pUserData);
}

void CMastersrv::AddCheckserver(const NETADDR *pAddr, const NETADDR *pAltAddr, void *pUserData, int Version)
{
	dbg_assert(Version >= 0 && Version < NUM_MASTERSRV, "version out of range");

	char aAddrStr[2 * NETADDR_MAXSTRSIZE + 3]; // 3 == sizeof(' ()')
	char aTmp1[NETADDR_MAXSTRSIZE], aTmp2[NETADDR_MAXSTRSIZE];

	net_addr_str(pAddr, aTmp1, sizeof(aTmp1), true);
	net_addr_str(pAltAddr, aTmp2, sizeof(aTmp2), true);

	str_format(aAddrStr, sizeof(aAddrStr), "%s (%s)", aTmp1, aTmp2);

	// see if server already exists in list
	for(int i = 0; i < m_NumCheckServers; i++)
	{
		if(net_addr_comp(&m_aCheckServers[i].m_Address, pAddr) == 0
			&& m_aCheckServers[i].m_Version == Version)
		{
			dbg_msg("mastersrv/check", "warning: updated: %s", aAddrStr);
			m_aCheckServers[i].m_AltAddress = *pAltAddr;
			m_aCheckServers[i].m_Version = Version;
			m_aCheckServers[i].m_pSlaveUserData = pUserData;
			m_aCheckServers[i].m_TryCount = 0;
			m_aCheckServers[i].m_TryTime = 0;
			return;
		}
	}

	// add server
	if(m_NumCheckServers == MAX_CHECKSERVERS)
	{
		dbg_msg("mastersrv/check", "error: mastersrv is full: %s", aAddrStr);
		return;
	}

	dbg_msg("mastersrv/check", "added: %s", aAddrStr);
	m_aCheckServers[m_NumCheckServers].m_Address = *pAddr;
	m_aCheckServers[m_NumCheckServers].m_AltAddress = *pAltAddr;
	m_aCheckServers[m_NumCheckServers].m_Version = Version;
	m_aCheckServers[m_NumCheckServers].m_pSlaveUserData = pUserData;
	m_aCheckServers[m_NumCheckServers].m_TryCount = 0;
	m_aCheckServers[m_NumCheckServers].m_TryTime = 0;
	m_NumCheckServers++;
}

void CMastersrv::SendList(const NETADDR *pAddr, void *pUserData, int Version)
{
	dbg_assert(Version >= 0 && Version < NUM_MASTERSRV, "version out of range");
	IMastersrvSlave *pSlave = m_aSlaves[Version].m_pSlave;
	dbg_assert(pSlave != 0, "attempting to access uninitalised slave");

	dbg_msg("mastersrv", "requested, responding with %d packets", m_aSlaves[Version].m_NumPackets);

	for(int i = 0; i < m_aSlaves[Version].m_NumPackets; i++)
	{
		CMastersrvSlave::CPacket *pPacket = &m_aSlaves[Version].m_aPackets[i];
		pSlave->SendList(pAddr, pPacket->m_aData, pPacket->m_Size, pUserData);
	}
}

int CMastersrv::GetCount() const
{
	dbg_msg("mastersrv", "requesting count, responding with %d", m_NumServers);
	return m_NumServers;
}

void CMastersrv::PurgeServers()
{
	int64 Now = time_get();
	int i = 0;
	while(i < m_NumServers)
	{
		if(m_aServers[i].m_Expire < Now)
		{
			// remove server
			char aAddrStr[NETADDR_MAXSTRSIZE];
			net_addr_str(&m_aServers[i].m_Address, aAddrStr, sizeof(aAddrStr), true);
			dbg_msg("mastersrv", "expired: %s", aAddrStr);
			m_aServers[i] = m_aServers[m_NumServers-1];
			m_NumServers--;
		}
		else
			i++;
	}
}

void CMastersrv::ReloadBans()
{
	m_NetBan.UnbanAll();
	m_pConsole->ExecuteFile("master.cfg");
}

/*enum
{
	MTU = 1400,
	MAX_SERVERS_PER_PACKET=75,
	MAX_PACKETS=16,
	MAX_SERVERS=MAX_SERVERS_PER_PACKET*MAX_PACKETS,
	EXPIRE_TIME = 90
};

struct CCheckServer
{
	enum ServerType m_Type;
	NETADDR m_Address;
	NETADDR m_AltAddress;
	int m_TryCount;
	int64 m_TryTime;
	TOKEN m_Token;
};

static CCheckServer m_aCheckServers[MAX_SERVERS];
static int m_NumCheckServers = 0;

struct CServerEntry
{
	enum ServerType m_Type;
	NETADDR m_Address;
	int64 m_Expire;
};

static CServerEntry m_aServers[MAX_SERVERS];
static int m_NumServers = 0;

struct CPacketData
{
	int m_Size;
	struct {
		unsigned char m_aHeader[sizeof(SERVERBROWSE_LIST)];
		CMastersrvAddr m_aServers[MAX_SERVERS_PER_PACKET];
	} m_Data;
};

CPacketData m_aPackets[MAX_PACKETS];
static int m_NumPackets = 0;

// legacy code
struct CPacketDataLegacy
{
	int m_Size;
	struct {
		unsigned char m_aHeader[sizeof(SERVERBROWSE_LIST_LEGACY)];
		CMastersrvAddrLegacy m_aServers[MAX_SERVERS_PER_PACKET];
	} m_Data;
};

CPacketDataLegacy m_aPacketsLegacy[MAX_PACKETS];
static int m_NumPacketsLegacy = 0;


struct CCountPacketData
{
	unsigned char m_Header[sizeof(SERVERBROWSE_COUNT)];
	unsigned char m_High;
	unsigned char m_Low;
};

static CCountPacketData m_CountData;
static CCountPacketData m_CountDataLegacy;


CNetBan m_NetBan;

static CNetClient m_NetChecker; // NAT/FW checker
static CNetClient m_NetOp; // main

IConsole *m_pConsole;

void BuildPackets()
{
	CServerEntry *pCurrent = &m_aServers[0];
	int ServersLeft = m_NumServers;
	m_NumPackets = 0;
	m_NumPacketsLegacy = 0;
	int PacketIndex = 0;
	int PacketIndexLegacy = 0;
	while(ServersLeft-- && (m_NumPackets + m_NumPacketsLegacy) < MAX_PACKETS)
	{
		if(pCurrent->m_Type == SERVERTYPE_NORMAL)
		{
			if(PacketIndex % MAX_SERVERS_PER_PACKET == 0)
			{
				PacketIndex = 0;
				m_NumPackets++;
			}

			// copy header
			mem_copy(m_aPackets[m_NumPackets-1].m_Data.m_aHeader, SERVERBROWSE_LIST, sizeof(SERVERBROWSE_LIST));

			// copy server addresses
			if(pCurrent->m_Address.type == NETTYPE_IPV6)
			{
				mem_copy(m_aPackets[m_NumPackets-1].m_Data.m_aServers[PacketIndex].m_aIp, pCurrent->m_Address.ip,
					sizeof(m_aPackets[m_NumPackets-1].m_Data.m_aServers[PacketIndex].m_aIp));
			}
			else
			{
				static unsigned char s_aIPV4Mapping[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF};

				mem_copy(m_aPackets[m_NumPackets-1].m_Data.m_aServers[PacketIndex].m_aIp, s_aIPV4Mapping, sizeof(s_aIPV4Mapping));
				m_aPackets[m_NumPackets-1].m_Data.m_aServers[PacketIndex].m_aIp[12] = pCurrent->m_Address.ip[0];
				m_aPackets[m_NumPackets-1].m_Data.m_aServers[PacketIndex].m_aIp[13] = pCurrent->m_Address.ip[1];
				m_aPackets[m_NumPackets-1].m_Data.m_aServers[PacketIndex].m_aIp[14] = pCurrent->m_Address.ip[2];
				m_aPackets[m_NumPackets-1].m_Data.m_aServers[PacketIndex].m_aIp[15] = pCurrent->m_Address.ip[3];
			}

			m_aPackets[m_NumPackets-1].m_Data.m_aServers[PacketIndex].m_aPort[0] = (pCurrent->m_Address.port>>8)&0xff;
			m_aPackets[m_NumPackets-1].m_Data.m_aServers[PacketIndex].m_aPort[1] = pCurrent->m_Address.port&0xff;

			PacketIndex++;

			m_aPackets[m_NumPackets-1].m_Size = sizeof(SERVERBROWSE_LIST) + sizeof(CMastersrvAddr)*PacketIndex;

			pCurrent++;
		}
		else if(pCurrent->m_Type == SERVERTYPE_LEGACY)
		{
			if(PacketIndexLegacy % MAX_SERVERS_PER_PACKET == 0)
			{
				PacketIndexLegacy = 0;
				m_NumPacketsLegacy++;
			}

			// copy header
			mem_copy(m_aPacketsLegacy[m_NumPacketsLegacy-1].m_Data.m_aHeader, SERVERBROWSE_LIST_LEGACY, sizeof(SERVERBROWSE_LIST_LEGACY));

			// copy server addresses
			mem_copy(m_aPacketsLegacy[m_NumPacketsLegacy-1].m_Data.m_aServers[PacketIndexLegacy].m_aIp, pCurrent->m_Address.ip,
				sizeof(m_aPacketsLegacy[m_NumPacketsLegacy-1].m_Data.m_aServers[PacketIndexLegacy].m_aIp));
			// 0.5 has the port in little endian on the network
			m_aPacketsLegacy[m_NumPacketsLegacy-1].m_Data.m_aServers[PacketIndexLegacy].m_aPort[0] = pCurrent->m_Address.port&0xff;
			m_aPacketsLegacy[m_NumPacketsLegacy-1].m_Data.m_aServers[PacketIndexLegacy].m_aPort[1] = (pCurrent->m_Address.port>>8)&0xff;

			PacketIndexLegacy++;

			m_aPacketsLegacy[m_NumPacketsLegacy-1].m_Size = sizeof(SERVERBROWSE_LIST_LEGACY) + sizeof(CMastersrvAddrLegacy)*PacketIndexLegacy;

			pCurrent++;
		}
		else
		{
			*pCurrent = m_aServers[m_NumServers-1];
			m_NumServers--;
			dbg_msg("mastersrv", "error: server of invalid type, dropping it");
		}
	}
}

void SendOk(NETADDR *pAddr, TOKEN Token)
{
	CNetChunk p;
	p.m_ClientID = -1;
	p.m_Address = *pAddr;
	p.m_Flags = NETSENDFLAG_CONNLESS;
	p.m_DataSize = sizeof(SERVERBROWSE_FWOK);
	p.m_pData = SERVERBROWSE_FWOK;

	// send on both to be sure
	m_NetChecker.Send(&p, Token);
	m_NetOp.Send(&p, Token);
}

void SendError(NETADDR *pAddr, TOKEN Token)
{
	CNetChunk p;
	p.m_ClientID = -1;
	p.m_Address = *pAddr;
	p.m_Flags = NETSENDFLAG_CONNLESS;
	p.m_DataSize = sizeof(SERVERBROWSE_FWERROR);
	p.m_pData = SERVERBROWSE_FWERROR;
	m_NetOp.Send(&p, Token);
}

void SendCheck(NETADDR *pAddr, TOKEN Token)
{
	CNetChunk p;
	p.m_ClientID = -1;
	p.m_Address = *pAddr;
	p.m_Flags = NETSENDFLAG_CONNLESS;
	p.m_DataSize = sizeof(SERVERBROWSE_FWCHECK);
	p.m_pData = SERVERBROWSE_FWCHECK;
	m_NetChecker.Send(&p, Token);
}

void AddCheckserver(NETADDR *pInfo, NETADDR *pAlt, ServerType Type, TOKEN Token)
{
	// add server
	if(m_NumCheckServers == MAX_SERVERS)
	{
		dbg_msg("mastersrv", "error: mastersrv is full");
		return;
	}

	char aAddrStr[NETADDR_MAXSTRSIZE];
	net_addr_str(pInfo, aAddrStr, sizeof(aAddrStr), true);
	char aAltAddrStr[NETADDR_MAXSTRSIZE];
	net_addr_str(pAlt, aAltAddrStr, sizeof(aAltAddrStr), true);
	dbg_msg("mastersrv", "checking: %s (%s)", aAddrStr, aAltAddrStr);
	m_aCheckServers[m_NumCheckServers].m_Address = *pInfo;
	m_aCheckServers[m_NumCheckServers].m_AltAddress = *pAlt;
	m_aCheckServers[m_NumCheckServers].m_TryCount = 0;
	m_aCheckServers[m_NumCheckServers].m_TryTime = 0;
	m_aCheckServers[m_NumCheckServers].m_Type = Type;
	m_aCheckServers[m_NumCheckServers].m_Token = Token;
	m_NumCheckServers++;
}

void AddServer(NETADDR *pInfo, ServerType Type)
{
	// see if server already exists in list
	for(int i = 0; i < m_NumServers; i++)
	{
		if(net_addr_comp(&m_aServers[i].m_Address, pInfo) == 0)
		{
			char aAddrStr[NETADDR_MAXSTRSIZE];
			net_addr_str(pInfo, aAddrStr, sizeof(aAddrStr), true);
			dbg_msg("mastersrv", "updated: %s", aAddrStr);
			m_aServers[i].m_Expire = time_get()+time_freq()*EXPIRE_TIME;
			return;
		}
	}

	// add server
	if(m_NumServers == MAX_SERVERS)
	{
		dbg_msg("mastersrv", "error: mastersrv is full");
		return;
	}

	char aAddrStr[NETADDR_MAXSTRSIZE];
	net_addr_str(pInfo, aAddrStr, sizeof(aAddrStr), true);
	dbg_msg("mastersrv", "added: %s", aAddrStr);
	m_aServers[m_NumServers].m_Address = *pInfo;
	m_aServers[m_NumServers].m_Expire = time_get()+time_freq()*EXPIRE_TIME;
	m_aServers[m_NumServers].m_Type = Type;
	m_NumServers++;
}

void UpdateServers()
{
	int64 Now = time_get();
	int64 Freq = time_freq();
	for(int i = 0; i < m_NumCheckServers; i++)
	{
		if(Now > m_aCheckServers[i].m_TryTime+Freq)
		{
			if(m_aCheckServers[i].m_TryCount == 10)
			{
				char aAddrStr[NETADDR_MAXSTRSIZE];
				net_addr_str(&m_aCheckServers[i].m_Address, aAddrStr, sizeof(aAddrStr), true);
				char aAltAddrStr[NETADDR_MAXSTRSIZE];
				net_addr_str(&m_aCheckServers[i].m_AltAddress, aAltAddrStr, sizeof(aAltAddrStr), true);
				dbg_msg("mastersrv", "check failed: %s (%s)", aAddrStr, aAltAddrStr);

				// FAIL!!
				SendError(&m_aCheckServers[i].m_Address, m_aCheckServers[i].m_Token);
				m_aCheckServers[i] = m_aCheckServers[m_NumCheckServers-1];
				m_NumCheckServers--;
				i--;
			}
			else
			{
				m_aCheckServers[i].m_TryCount++;
				m_aCheckServers[i].m_TryTime = Now;
				if(m_aCheckServers[i].m_TryCount&1)
					SendCheck(&m_aCheckServers[i].m_Address, m_aCheckServers[i].m_Token);
				else
					SendCheck(&m_aCheckServers[i].m_AltAddress, m_aCheckServers[i].m_Token);
			}
		}
	}
}

void PurgeServers()
{
	int64 Now = time_get();
	int i = 0;
	while(i < m_NumServers)
	{
		if(m_aServers[i].m_Expire < Now)
		{
			// remove server
			char aAddrStr[NETADDR_MAXSTRSIZE];
			net_addr_str(&m_aServers[i].m_Address, aAddrStr, sizeof(aAddrStr), true);
			dbg_msg("mastersrv", "expired: %s", aAddrStr);
			m_aServers[i] = m_aServers[m_NumServers-1];
			m_NumServers--;
		}
		else
			i++;
	}
}

void ReloadBans()
{
	m_NetBan.UnbanAll();
	m_pConsole->ExecuteFile("master.cfg");
}

int main(int argc, const char **argv) // ignore_convention
{
	int64 LastBuild = 0, LastBanReload = 0;
	ServerType Type = SERVERTYPE_INVALID;
	NETADDR BindAddr;

	dbg_logger_stdout();
	net_init();

	mem_copy(m_CountData.m_Header, SERVERBROWSE_COUNT, sizeof(SERVERBROWSE_COUNT));
	mem_copy(m_CountDataLegacy.m_Header, SERVERBROWSE_COUNT_LEGACY, sizeof(SERVERBROWSE_COUNT_LEGACY));

	IKernel *pKernel = IKernel::Create();
	IStorage *pStorage = CreateStorage("Teeworlds", IStorage::STORAGETYPE_BASIC, argc, argv);
	IConfig *pConfig = CreateConfig();
	m_pConsole = CreateConsole(CFGFLAG_MASTER);
	
	bool RegisterFail = !pKernel->RegisterInterface(pStorage);
	RegisterFail |= !pKernel->RegisterInterface(m_pConsole);
	RegisterFail |= !pKernel->RegisterInterface(pConfig);

	if(RegisterFail)
		return -1;

	pConfig->Init();
	m_NetBan.Init(m_pConsole, pStorage);
	if(argc > 1) // ignore_convention
		m_pConsole->ParseArguments(argc-1, &argv[1]); // ignore_convention

	if(g_Config.m_Bindaddr[0] && net_host_lookup(g_Config.m_Bindaddr, &BindAddr, NETTYPE_ALL) == 0)
	{
		// got bindaddr
		BindAddr.type = NETTYPE_ALL;
		BindAddr.port = MASTERSERVER_PORT;
	}
	else
	{
		mem_zero(&BindAddr, sizeof(BindAddr));
		BindAddr.type = NETTYPE_ALL;
		BindAddr.port = MASTERSERVER_PORT;
	}

	if(!m_NetOp.Open(BindAddr, NETFLAG_ALLOWSTATELESS))
	{
		dbg_msg("mastersrv", "couldn't start network (op)");
		return -1;
	}
	BindAddr.port = MASTERSERVER_PORT+1;
	if(!m_NetChecker.Open(BindAddr, NETFLAG_ALLOWSTATELESS))
	{
		dbg_msg("mastersrv", "couldn't start network (checker)");
		return -1;
	}

	// process pending commands
	m_pConsole->StoreCommands(false);

	dbg_msg("mastersrv", "started");

	while(1)
	{
		m_NetOp.Update();
		m_NetChecker.Update();

		// process m_aPackets
		CNetChunk Packet;
		TOKEN Token;
		while(m_NetOp.Recv(&Packet, &Token))
		{
			// check if the server is banned
			if(m_NetBan.IsBanned(&Packet.m_Address, 0, 0))
				continue;

			if(Packet.m_DataSize == sizeof(SERVERBROWSE_HEARTBEAT)+2 &&
				mem_comp(Packet.m_pData, SERVERBROWSE_HEARTBEAT, sizeof(SERVERBROWSE_HEARTBEAT)) == 0)
			{
				NETADDR Alt;
				unsigned char *d = (unsigned char *)Packet.m_pData;
				Alt = Packet.m_Address;
				Alt.port =
					(d[sizeof(SERVERBROWSE_HEARTBEAT)]<<8) |
					d[sizeof(SERVERBROWSE_HEARTBEAT)+1];

				// add it
				AddCheckserver(&Packet.m_Address, &Alt, SERVERTYPE_NORMAL, Token);
			}
			else if(Packet.m_DataSize == sizeof(SERVERBROWSE_HEARTBEAT_LEGACY)+2 &&
				mem_comp(Packet.m_pData, SERVERBROWSE_HEARTBEAT_LEGACY, sizeof(SERVERBROWSE_HEARTBEAT_LEGACY)) == 0)
			{
				NETADDR Alt;
				unsigned char *d = (unsigned char *)Packet.m_pData;
				Alt = Packet.m_Address;
				Alt.port =
					(d[sizeof(SERVERBROWSE_HEARTBEAT)]<<8) |
					d[sizeof(SERVERBROWSE_HEARTBEAT)+1];

				// add it
				AddCheckserver(&Packet.m_Address, &Alt, SERVERTYPE_LEGACY);
			}
			else if(Packet.m_DataSize == sizeof(SERVERBROWSE_GETCOUNT) &&
				mem_comp(Packet.m_pData, SERVERBROWSE_GETCOUNT, sizeof(SERVERBROWSE_GETCOUNT)) == 0)
			{
				dbg_msg("mastersrv", "count requested, responding with %d", m_NumServers);

				CNetChunk p;
				p.m_ClientID = -1;
				p.m_Address = Packet.m_Address;
				p.m_Flags = NETSENDFLAG_CONNLESS;
				p.m_DataSize = sizeof(m_CountData);
				p.m_pData = &m_CountData;
				m_CountData.m_High = (m_NumServers>>8)&0xff;
				m_CountData.m_Low = m_NumServers&0xff;
				m_NetOp.Send(&p, Token);
			}
			else if(Packet.m_DataSize == sizeof(SERVERBROWSE_GETCOUNT_LEGACY) &&
				mem_comp(Packet.m_pData, SERVERBROWSE_GETCOUNT_LEGACY, sizeof(SERVERBROWSE_GETCOUNT_LEGACY)) == 0)
			{
				dbg_msg("mastersrv", "count requested, responding with %d", m_NumServers);

				CNetChunk p;
				p.m_ClientID = -1;
				p.m_Address = Packet.m_Address;
				p.m_Flags = NETSENDFLAG_CONNLESS;
				p.m_DataSize = sizeof(m_CountData);
				p.m_pData = &m_CountDataLegacy;
				m_CountDataLegacy.m_High = (m_NumServers>>8)&0xff;
				m_CountDataLegacy.m_Low = m_NumServers&0xff;
				m_NetOp.Send(&p);
			}
			else if(Packet.m_DataSize == sizeof(SERVERBROWSE_GETLIST) &&
				mem_comp(Packet.m_pData, SERVERBROWSE_GETLIST, sizeof(SERVERBROWSE_GETLIST)) == 0)
			{
				// someone requested the list
				dbg_msg("mastersrv", "requested, responding with %d m_aServers", m_NumServers);

				CNetChunk p;
				p.m_ClientID = -1;
				p.m_Address = Packet.m_Address;
				p.m_Flags = NETSENDFLAG_CONNLESS;

				for(int i = 0; i < m_NumPackets; i++)
				{
					p.m_DataSize = m_aPackets[i].m_Size;
					p.m_pData = &m_aPackets[i].m_Data;
					m_NetOp.Send(&p, Token);
				}
			}
			else if(Packet.m_DataSize == sizeof(SERVERBROWSE_GETLIST_LEGACY) &&
				mem_comp(Packet.m_pData, SERVERBROWSE_GETLIST_LEGACY, sizeof(SERVERBROWSE_GETLIST_LEGACY)) == 0)
			{
				// someone requested the list
				dbg_msg("mastersrv", "requested, responding with %d m_aServers", m_NumServers);

				CNetChunk p;
				p.m_ClientID = -1;
				p.m_Address = Packet.m_Address;
				p.m_Flags = NETSENDFLAG_CONNLESS;

				for(int i = 0; i < m_NumPacketsLegacy; i++)
				{
					p.m_DataSize = m_aPacketsLegacy[i].m_Size;
					p.m_pData = &m_aPacketsLegacy[i].m_Data;
					m_NetOp.Send(&p);
				}
			}
		}

		// process m_aPackets
		while(m_NetChecker.Recv(&Packet, &Token))
		{
			// check if the server is banned
			if(m_NetBan.IsBanned(&Packet.m_Address, 0, 0))
				continue;

			if(Packet.m_DataSize == sizeof(SERVERBROWSE_FWRESPONSE) &&
				mem_comp(Packet.m_pData, SERVERBROWSE_FWRESPONSE, sizeof(SERVERBROWSE_FWRESPONSE)) == 0)
			{
				Type = SERVERTYPE_INVALID;
				// remove it from checking
				for(int i = 0; i < m_NumCheckServers; i++)
				{
					if(net_addr_comp(&m_aCheckServers[i].m_Address, &Packet.m_Address) == 0 ||
						net_addr_comp(&m_aCheckServers[i].m_AltAddress, &Packet.m_Address) == 0)
					{
						Type = m_aCheckServers[i].m_Type;
						m_NumCheckServers--;
						m_aCheckServers[i] = m_aCheckServers[m_NumCheckServers];
						break;
					}
				}

				// drops servers that were not in the CheckServers list
				if(Type == SERVERTYPE_INVALID)
					continue;

				AddServer(&Packet.m_Address, Type);
				SendOk(&Packet.m_Address, Token);
			}
		}

		if(time_get()-LastBanReload > time_freq()*300)
		{
			LastBanReload = time_get();

			ReloadBans();
		}

		if(time_get()-LastBuild > time_freq()*5)
		{
			LastBuild = time_get();

			PurgeServers();
			UpdateServers();
			BuildPackets();
		}

		// be nice to the CPU
		thread_sleep(1);
	}

	return 0;
}
*/

