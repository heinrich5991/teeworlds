
#include <engine/server.h>

#include <engine/shared/network.h>
#include <engine/shared/packer.h>
#include <engine/shared/protocol.h>
#include <engine/shared/snapshot.h>

#include <game/generated/protocol.h>

#include "0.5/nethash.h"
#include "0.5/protocol.h"
#include "0.5/mastersrv.h"

#include "0.5-0.6/proxy.h"

#include "proxy.h"

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
		DIR_SEND=0,
		DIR_RECV,
		NUM_DIRS,
	};

public:
	CHacksServer();
	virtual ~CHacksServer();

	virtual void Init();
	virtual int GetSendPacket(CNetChunk *pPacket) { return GetPacket(pPacket, DIR_SEND); }
	virtual int GetRecvPacket(CNetChunk *pPacket) { return GetPacket(pPacket, DIR_RECV); }
	virtual int OnSendPacket(CNetChunk *pPacket) { return OnPacket(pPacket, DIR_SEND); }
	virtual int OnRecvPacket(CNetChunk *pPacket) { return OnPacket(pPacket, DIR_RECV); }

	virtual void OnSnap(int ClientID, CSnapshot *pSnap, int *pSnapSize);
	virtual void OnDisconnect(int ClientID);

	virtual void SetNet(void *pNet) { m_pNet = (CNetServer *)pNet; }

	int GetPacket(CNetChunk *pPacket, int Dir);
	int OnPacket(CNetChunk *pPacket, int Dir);

	void TranslatePacketCBImpl(CNetChunk *pPacket);
	static void TranslatePacketCB(CNetChunk *pPacket, void *pUserData)
	{
		((CHacksServer *)pUserData)->TranslatePacketCBImpl(pPacket);
	}

	void InitCBData(int Dir, int ClientID);
	void FinalizeCBData(int Dir);

	int Detect5(CNetChunk *pPacket);
	bool IsConnless5(CNetChunk *pPacket);
	void SetProxy5(int ClientID);

private:
	enum
	{
		PROXY_05_06=0,
		PROXY_06_05,
		NUM_PROXIES,

		MAX_PACKETS_PER_PACKET=16,
	};

	struct CClient
	{
		NETADDR m_Addr;
		IProxy *m_apProxies[NUM_DIRS];
	};
	IProxy *m_apProxies[NUM_PROXIES];
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
	int m_CBDir;

	NETADDR m_ConnlessAddr;
	IProxy *m_pConnlessAddrProxy;
};

IHacks *CreateHacks() { return new CHacksServer(); }

CHacksServer::CHacksServer()
{
	mem_zero(m_aClients, sizeof(m_aClients));
	mem_zero(m_aCBData, sizeof(m_aCBData));
	m_CBDir = -1;
	m_apProxies[PROXY_05_06] = CreateProxy_05_06(this, TranslatePacketCB, this);
	m_apProxies[PROXY_06_05] = CreateProxy_06_05(this, TranslatePacketCB, this);
	m_pConnlessAddrProxy = 0;
}

CHacksServer::~CHacksServer()
{
	for(int i = 0; i < NUM_PROXIES; i++)
		delete m_apProxies[i];

	mem_zero(m_apProxies, sizeof(m_apProxies));
	mem_zero(m_aClients, sizeof(m_aClients));
	m_pConnlessAddrProxy = 0;
}

void CHacksServer::Init()
{
	m_pServer = Kernel()->RequestInterface<IServer>();
}

void CHacksServer::InitCBData(int Dir, int ClientID)
{
	dbg_assert(0 <= Dir && Dir < NUM_DIRS, "invalid dir");
	dbg_assert(!m_aCBData[Dir].m_Active, "must not be active at that time");
	dbg_assert(m_aCBData[Dir].m_NumPackets == 0, "cannot init, not empty");

	//mem_zero(m_aCBData[Dir], sizeof(m_aCBData[Dir]));
	m_aCBData[Dir].m_ClientID = ClientID;
	m_aCBData[Dir].m_Offset = 0;
	m_aCBData[Dir].m_Active = true;

	m_CBDir = Dir;
}

void CHacksServer::FinalizeCBData(int Dir)
{
	dbg_assert(m_aCBData[Dir].m_Active, "must be active at that time");
	m_aCBData[Dir].m_Active = false;
}

void CHacksServer::TranslatePacketCBImpl(CNetChunk *pPacket)
{
	dbg_assert(m_aCBData[m_CBDir].m_Active, "must be active at that time");
	dbg_assert(m_aCBData[m_CBDir].m_NumPackets < MAX_PACKETS_PER_PACKET, "too many packets per packet");

	CCBData *pCBData = &m_aCBData[m_CBDir];
	int NumPackets = pCBData->m_NumPackets;

	pCBData->m_aPacketData[NumPackets].Reset();
	pCBData->m_aPacketData[NumPackets].AddRaw(pPacket->m_pData, pPacket->m_DataSize);
	pCBData->m_aPackets[NumPackets] = *pPacket;
	pCBData->m_aPackets[NumPackets].m_pData = pCBData->m_aPacketData[NumPackets].Data();
	pCBData->m_NumPackets++;
}

int CHacksServer::OnPacket(CNetChunk *pPacket, int Dir)
{
	int ClientID = pPacket->m_ClientID;
	dbg_assert(-1 <= ClientID && ClientID < MAX_CLIENTS, "cid out of range");

	if(pPacket->m_Flags&NETSENDFLAG_CONNLESS)
	{
		// 0.5 begin
		if(Dir == DIR_RECV && IsConnless5(pPacket))
		{
			InitCBData(Dir, ClientID);
			m_apProxies[PROXY_05_06]->TranslatePacket(pPacket);
			FinalizeCBData(Dir);
			return 1;
		}
		// 0.5 end

		if(Dir == DIR_SEND)
		{
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				if(m_aClients[ClientID].m_apProxies[DIR_SEND]
					&& net_addr_comp(&m_aClients[ClientID].m_Addr, &pPacket->m_Address) == 0)
				{
					InitCBData(Dir, ClientID);
					m_aClients[ClientID].m_apProxies[DIR_SEND]->TranslatePacket(pPacket);
					FinalizeCBData(Dir);
					return 1;
				}
			}
		}

		// connlesshack begin
		if(Dir == DIR_SEND && m_pConnlessAddrProxy
			&& net_addr_comp(&pPacket->m_Address, &m_ConnlessAddr) == 0)
		{
			InitCBData(Dir, ClientID);
			m_apProxies[PROXY_06_05]->TranslatePacket(pPacket);
			FinalizeCBData(Dir);
			return 1;
		}
		// connlesshack end

		return 0;
	}

	dbg_assert(0 <= ClientID && ClientID < MAX_CLIENTS, "cid out of range");
	if(m_aClients[ClientID].m_apProxies[Dir])
	{
		InitCBData(Dir, ClientID);
		m_aClients[ClientID].m_apProxies[Dir]->TranslatePacket(pPacket);
		FinalizeCBData(Dir);
		return 1;
	}

	// 0.5 begin
	if(Dir == DIR_RECV)
	{
		int Result = Detect5(pPacket);
		if(Result)
			return Result;
	}
	// 0.5 end

	return 0;
}

int CHacksServer::GetPacket(CNetChunk *pPacket, int Dir)
{
	dbg_assert(!m_aCBData[Dir].m_Active, "must be inactive at that time");

	if(m_aCBData[Dir].m_NumPackets == 0)
		return 0;

	*pPacket = m_aCBData[Dir].m_aPackets[m_aCBData[Dir].m_Offset];
	m_aCBData[Dir].m_Offset++;
	m_aCBData[Dir].m_NumPackets--;

	// connlesshack begin
	if(Dir == DIR_RECV)
	{
		if(pPacket->m_Flags&NETSENDFLAG_CONNLESS)
		{
			m_ConnlessAddr = pPacket->m_Address;
			m_pConnlessAddrProxy = m_apProxies[PROXY_06_05];
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
				SetProxy5(ClientID);
				return OnRecvPacket(pPacket);
			}
		}
	}

	return 0;
}
// 0.5 end

// 0.5 begin
bool CHacksServer::IsConnless5(CNetChunk *pPacket)
{
	if(pPacket->m_DataSize == sizeof(Protocol5::SERVERBROWSE_GETINFO) + 1
		&& mem_comp(pPacket->m_pData, Protocol5::SERVERBROWSE_GETINFO,
			    sizeof(Protocol5::SERVERBROWSE_GETINFO)) == 0)
		return true;


	char aBuf[128];
	str_hex(aBuf, sizeof(aBuf), pPacket->m_pData, pPacket->m_DataSize-1);
	dbg_msg("proxy/is5", "got msg data=%s", aBuf);
	str_hex(aBuf, sizeof(aBuf), Protocol5::SERVERBROWSE_GETINFO, sizeof(Protocol5::SERVERBROWSE_GETINFO));
	dbg_msg("proxy/is5", "      wanted=%s", aBuf);

	return false;
}
// 0.5 end

void CHacksServer::SetProxy5(int ClientID)
{
	dbg_assert(0 <= ClientID && ClientID < MAX_CLIENTS, "cid out of range");
	m_aClients[ClientID].m_apProxies[DIR_SEND] = m_apProxies[PROXY_06_05];
	m_aClients[ClientID].m_apProxies[DIR_RECV] = m_apProxies[PROXY_05_06];
	m_aClients[ClientID].m_Addr = m_pNet->ClientAddr(ClientID);
	dbg_msg("proxy", "0.5 detected cid=%x", ClientID);
}

void CHacksServer::OnSnap(int ClientID, CSnapshot *pSnap, int *pSnapSize)
{
	dbg_assert(0 <= ClientID && ClientID < MAX_CLIENTS, "cid out of range"); // proxy: TODO: check that before release
	if(m_aClients[ClientID].m_apProxies[DIR_SEND])
	{
		*pSnapSize = m_aClients[ClientID].m_apProxies[DIR_SEND]->TranslateSnap(pSnap);
	}
}

void CHacksServer::OnDisconnect(int ClientID)
{
	if(m_aClients[ClientID].m_apProxies[DIR_SEND]
		|| m_aClients[ClientID].m_apProxies[DIR_RECV])
		dbg_msg("proxy", "proxy removed cid=%x", ClientID);
	mem_zero(&m_aClients[ClientID], sizeof(m_aClients[ClientID]));
}

