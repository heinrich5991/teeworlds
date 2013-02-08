
#include <engine/server.h>

#include <engine/shared/network.h>
#include <engine/shared/packer.h>
#include <engine/shared/protocol.h>
#include <engine/shared/snapshot.h>

#include <game/generated/protocol.h>

#include "0.5/nethash.h"
#include "0.5/protocol.h"
#include "0.5/mastersrv.h"

#include "proxy.h"

#include "hacks.h"

class CHacksServer : public IHacks
{
private:
	IServer *m_pServer;
	CNetServer *m_pNet;
public:
	IServer *Server() { return m_pServer; }

private:
	enum
	{
		ORIGIN_SERVER=0,
		ORIGIN_CLIENT,
		NUM_DIRS,
	};

public:
	CHacksServer();
	virtual ~CHacksServer();

	virtual void Init();
	virtual int GetSendPacket(CNetChunk *pPacket) { return GetPacket(pPacket, ORIGIN_SERVER); }
	virtual int GetRecvPacket(CNetChunk *pPacket) { return GetPacket(pPacket, ORIGIN_CLIENT); }
	virtual int OnSendPacket(CNetChunk *pPacket) { return OnPacket(pPacket, ORIGIN_SERVER); }
	virtual int OnRecvPacket(CNetChunk *pPacket) { return OnPacket(pPacket, ORIGIN_CLIENT); }

	virtual void OnSnap(int ClientID, CSnapshot *pSnap, int *pSnapSize);
	virtual void OnDisconnect(int ClientID);

	virtual void SetNet(void *pNet) { m_pNet = (CNetServer *)pNet; }

	int GetPacket(CNetChunk *pPacket, int Origin);
	int OnPacket(CNetChunk *pPacket, int Origin);

	void TranslatePacketCBImpl(CNetChunk *pPacket);
	static void TranslatePacketCB(CNetChunk *pPacket, void *pUserData)
	{
		((CHacksServer *)pUserData)->TranslatePacketCBImpl(pPacket);
	}

	void InitCBData(int Origin, int ClientID);
	void FinalizeCBData(int Origin);

	int Detect5(CNetChunk *pPacket);

	int GetConnlessVersion(CNetChunk *pPacket);
	void SetProxy(int ClientID, int Version);

	int GetServerVersion() { return VERSION_06; }

private:
	enum
	{
		MAX_PACKETS_PER_PACKET=16,
	};

	struct CClient
	{
		NETADDR m_Addr;
		IProxy *m_pProxy;
	};

	CClient m_aClients[MAX_CLIENTS];

	struct CCBData
	{
		bool m_Active;
		int m_ClientID;
		CNetChunk m_aPackets[MAX_PACKETS_PER_PACKET];
		CPacker m_aPacketData[MAX_PACKETS_PER_PACKET];
		int m_NumPackets;
		int m_Offset;
	};

	CCBData m_aCBData[NUM_DIRS];
	int m_CBOrigin;

	NETADDR m_ConnlessAddr;
	IProxy *m_pConnlessAddrProxy;

	IProxy *m_apConnlessProxies[NUM_VERSIONS];
};

IHacks *CreateHacks() { return new CHacksServer(); }

CHacksServer::CHacksServer()
{
	mem_zero(m_aClients, sizeof(m_aClients));
	mem_zero(m_aCBData, sizeof(m_aCBData));
	mem_zero(&m_ConnlessAddr, sizeof(m_ConnlessAddr));
	m_pConnlessAddrProxy = 0;
	m_CBOrigin = -1;

	m_apConnlessProxies[VERSION_05] = CreateProxy(GetServerVersion(), VERSION_05, this, TranslatePacketCB, this);
	m_apConnlessProxies[VERSION_06] = 0;
}

CHacksServer::~CHacksServer()
{
	mem_zero(m_apConnlessProxies, sizeof(m_apConnlessProxies));
	mem_zero(m_aClients, sizeof(m_aClients));
	m_pConnlessAddrProxy = 0;
}

void CHacksServer::Init()
{
	m_pServer = Kernel()->RequestInterface<IServer>();
}

void CHacksServer::InitCBData(int Origin, int ClientID)
{
	dbg_assert(0 <= Origin && Origin < NUM_DIRS, "invalid origin");
	dbg_assert(!m_aCBData[Origin].m_Active, "must not be active at that time");
	dbg_assert(m_aCBData[Origin].m_NumPackets == 0, "cannot init, not empty");

	//mem_zero(m_aCBData[Origin], sizeof(m_aCBData[Origin]));
	m_aCBData[Origin].m_ClientID = ClientID;
	m_aCBData[Origin].m_Offset = 0;
	m_aCBData[Origin].m_Active = true;

	m_CBOrigin = Origin;
}

void CHacksServer::FinalizeCBData(int Origin)
{
	dbg_assert(m_aCBData[Origin].m_Active, "must be active at that time");
	m_aCBData[Origin].m_Active = false;
}

void CHacksServer::TranslatePacketCBImpl(CNetChunk *pPacket)
{
	dbg_assert(m_aCBData[m_CBOrigin].m_Active, "must be active at that time");
	dbg_assert(m_aCBData[m_CBOrigin].m_NumPackets < MAX_PACKETS_PER_PACKET, "too many packets per packet");

	CCBData *pCBData = &m_aCBData[m_CBOrigin];
	int NumPackets = pCBData->m_NumPackets;

	pCBData->m_aPacketData[NumPackets].Reset();
	pCBData->m_aPacketData[NumPackets].AddRaw(pPacket->m_pData, pPacket->m_DataSize);
	pCBData->m_aPackets[NumPackets] = *pPacket;
	pCBData->m_aPackets[NumPackets].m_pData = pCBData->m_aPacketData[NumPackets].Data();
	pCBData->m_NumPackets++;
}

int CHacksServer::OnPacket(CNetChunk *pPacket, int Origin)
{
	int ClientID = pPacket->m_ClientID;
	dbg_assert(-1 <= ClientID && ClientID < MAX_CLIENTS, "cid out of range");

	if(pPacket->m_Flags&NETSENDFLAG_CONNLESS)
	{
		// connlesshack begin
		if(Origin == ORIGIN_CLIENT)
		{
			int Version = GetConnlessVersion(pPacket);
			if(m_apConnlessProxies[Version])
			{
				InitCBData(Origin, ClientID);
				m_apConnlessProxies[Version]->TranslatePacket(pPacket, Origin);
				FinalizeCBData(Origin);
				return 1;
			}
			else
				return 0;
		}
		// 0.5 end

		if(Origin == ORIGIN_SERVER)
		{
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				if(m_aClients[ClientID].m_pProxy
					&& net_addr_comp(&m_aClients[ClientID].m_Addr, &pPacket->m_Address) == 0)
				{
					InitCBData(Origin, ClientID);
					m_aClients[ClientID].m_pProxy->TranslatePacket(pPacket, Origin);
					FinalizeCBData(Origin);
					return 1;
				}
			}
		}

		// connlesshack begin
		if(Origin == ORIGIN_SERVER && m_pConnlessAddrProxy
			&& net_addr_comp(&pPacket->m_Address, &m_ConnlessAddr) == 0)
		{
			InitCBData(Origin, ClientID);
			m_apConnlessProxies[VERSION_05]->TranslatePacket(pPacket, Origin); // proxy: TODO: fix this
			FinalizeCBData(Origin);
			return 1;
		}
		// connlesshack end

		return 0;
	}

	dbg_assert(0 <= ClientID && ClientID < MAX_CLIENTS, "cid out of range");
	if(m_aClients[ClientID].m_pProxy)
	{
		InitCBData(Origin, ClientID);
		m_aClients[ClientID].m_pProxy->TranslatePacket(pPacket, Origin);
		FinalizeCBData(Origin);
		return 1;
	}

	// 0.5 begin
	if(Origin == ORIGIN_CLIENT)
	{
		int Result = Detect5(pPacket);
		if(Result)
			return Result;
	}
	// 0.5 end

	return 0;
}

int CHacksServer::GetPacket(CNetChunk *pPacket, int Origin)
{
	dbg_assert(!m_aCBData[Origin].m_Active, "must be inactive at that time");

	if(m_aCBData[Origin].m_NumPackets == 0)
		return 0;

	*pPacket = m_aCBData[Origin].m_aPackets[m_aCBData[Origin].m_Offset];
	m_aCBData[Origin].m_Offset++;
	m_aCBData[Origin].m_NumPackets--;

	// connlesshack begin
	if(Origin == ORIGIN_CLIENT)
	{
		if(pPacket->m_Flags&NETSENDFLAG_CONNLESS)
		{
			m_ConnlessAddr = pPacket->m_Address;
			m_pConnlessAddrProxy = m_apConnlessProxies[VERSION_05];
		}
		else
			m_pConnlessAddrProxy = 0;
	}
	// connlesshack end

	return 1;
}

// 0.5 begin
int CHacksServer::Detect5(CNetChunk *pPacket)
{
	if(pPacket->m_Flags&NETSENDFLAG_CONNLESS)
		return 0;

	int ClientID = pPacket->m_ClientID;
	dbg_assert(0 <= ClientID && ClientID < MAX_CLIENTS, "cid out of range");

	if(!Server()->ClientIngame(ClientID))
	{
		CUnpacker Unpacker;
		Unpacker.Reset(pPacket->m_pData, pPacket->m_DataSize);

		int Msg = Unpacker.GetInt();
		int Sys = Msg&1;
		Msg >>= 1;

		dbg_msg("proxy/det5", "packet sys=%d msg=%d", Sys, Msg);

		if(Unpacker.Error())
		{
			dbg_msg("proxy", "shouldn't happen"); // proxy: TODO: remove this
			return false;
		}

		if(Sys && Msg == Protocol5::NETMSG_INFO)
		{
			const char *pVersion = Unpacker.GetString(CUnpacker::SANITIZE_CC);
			if(str_comp(pVersion, Protocol5::GAME_NETVERSION) == 0)
			{
				SetProxy(ClientID, VERSION_05);
				return OnRecvPacket(pPacket);
			}
		}
	}

	return 0;
}
// 0.5 end

int CHacksServer::GetConnlessVersion(CNetChunk *pPacket)
{
	// 0.5 begin
	if(pPacket->m_DataSize == sizeof(Protocol5::SERVERBROWSE_GETINFO) + 1
		&& mem_comp(pPacket->m_pData, Protocol5::SERVERBROWSE_GETINFO,
			    sizeof(Protocol5::SERVERBROWSE_GETINFO)) == 0)
		return VERSION_05;
	// 0.5 end

	return VERSION_06;
}

void CHacksServer::SetProxy(int ClientID, int Version)
{
	dbg_assert(0 <= ClientID && ClientID < MAX_CLIENTS, "cid out of range");
	dbg_assert(0 <= Version && Version < NUM_VERSIONS, "invalid version");

	if(Version == GetServerVersion())
	{
		m_aClients[ClientID].m_pProxy = 0;
		return;
	}

	m_aClients[ClientID].m_pProxy = CreateProxy(GetServerVersion(), Version, this, TranslatePacketCB, this);
	m_aClients[ClientID].m_Addr = m_pNet->ClientAddr(ClientID);

	if(Version == VERSION_05)
		dbg_msg("proxy", "0.5 detected cid=%d", ClientID);
}

void CHacksServer::OnSnap(int ClientID, CSnapshot *pSnap, int *pSnapSize)
{
	dbg_assert(0 <= ClientID && ClientID < MAX_CLIENTS, "cid out of range"); // proxy: TODO: check that before release
	if(m_aClients[ClientID].m_pProxy)
	{
		*pSnapSize = m_aClients[ClientID].m_pProxy->TranslateServerSnap(pSnap);
	}
}

void CHacksServer::OnDisconnect(int ClientID)
{
	if(m_aClients[ClientID].m_pProxy)
	{
		dbg_msg("proxy", "proxy removed cid=%d", ClientID);
		delete m_aClients[ClientID].m_pProxy;
	}
	mem_zero(&m_aClients[ClientID], sizeof(m_aClients[ClientID]));
}

