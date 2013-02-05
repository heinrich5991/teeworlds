
#include <engine/server.h>

#include <engine/shared/network.h>
#include <engine/shared/packer.h>
#include <engine/shared/protocol.h>
#include <engine/shared/snapshot.h>

#include <game/generated/protocol.h>

#include "0.5/nethash.h"
#include "0.5/protocol.h"

#include "0.5-0.6/proxy.h"

#include "proxy.h"

class CHacksServer : public IHacks
{
private:
	IServer *m_pServer;
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

private:
	enum
	{
		PROXY_05_06=0,
		PROXY_06_05,
		NUM_PROXIES,

		MAX_PACKETS_PER_PACKET=16,
	};

	IProxy *m_apProxies[NUM_PROXIES];
	IProxy *m_apClientProxies[MAX_CLIENTS][NUM_DIRS];

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
};

IHacks *CreateHacks() { return new CHacksServer(); }

CHacksServer::CHacksServer()
{
	mem_zero(m_apClientProxies, sizeof(m_apClientProxies));
	mem_zero(m_aCBData, sizeof(m_aCBData));
	m_CBDir = -1;
	m_apProxies[PROXY_05_06] = CreateProxy_05_06(this, TranslatePacketCB, this);
	m_apProxies[PROXY_06_05] = CreateProxy_06_05(this, TranslatePacketCB, this);
}

CHacksServer::~CHacksServer()
{
	for(int i = 0; i < NUM_PROXIES; i++)
		delete m_apProxies[i];

	mem_zero(m_apProxies, sizeof(m_apProxies));
	mem_zero(m_apClientProxies, sizeof(m_apClientProxies));
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

	if(ClientID < 0)
		return 0; // proxy: TODO: add handling for connless packets

	if(m_apClientProxies[ClientID][Dir])
	{
		InitCBData(Dir, ClientID);
		m_apClientProxies[ClientID][Dir]->TranslatePacket(pPacket);
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
				m_apClientProxies[ClientID][DIR_SEND] = m_apProxies[PROXY_06_05];
				m_apClientProxies[ClientID][DIR_RECV] = m_apProxies[PROXY_05_06];
				dbg_msg("proxy", "0.5 detected cid=%x", ClientID);
				return OnRecvPacket(pPacket);
			}
		}
	}

	return 0;
}
// 0.5 end

void CHacksServer::OnSnap(int ClientID, CSnapshot *pSnap, int *pSnapSize)
{
	dbg_assert(0 <= ClientID && ClientID < MAX_CLIENTS, "cid out of range"); // proxy: TODO: check that before release
	if(m_apClientProxies[ClientID][DIR_SEND])
	{
		*pSnapSize = m_apClientProxies[ClientID][DIR_SEND]->TranslateSnap(pSnap);
	}
}

void CHacksServer::OnDisconnect(int ClientID)
{
	if(m_apClientProxies[ClientID][DIR_SEND]
		|| m_apClientProxies[ClientID][DIR_RECV])
		dbg_msg("proxy", "proxy removed cid=%x", ClientID);
	m_apClientProxies[ClientID][DIR_SEND] = 0;
	m_apClientProxies[ClientID][DIR_RECV] = 0;
}

