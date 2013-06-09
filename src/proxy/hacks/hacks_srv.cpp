
#include <engine/server.h>
#include <engine/console.h>

#include <engine/shared/network.h>
#include <engine/shared/packer.h>
#include <engine/shared/protocol.h>
#include <engine/shared/snapshot.h>

#include <game/generated/protocol.h>

#include <proxy/proxy/0.5/nethash.h>
#include <proxy/proxy/0.5/protocol.h>
#include <proxy/proxy/0.5/mastersrv.h>

#include <proxy/proxy/0.6/nethash.h>

#include <proxy/proxy/proxy.h>

#include "hacks.h"

#include "hacks_register.h"

class CHacksServer : public CHacks
{
private:
	IServer *m_pServer;
	CNetServer *m_pNet;
	CHacksRegister m_Register;
public:
	IServer *Server() { return m_pServer; }

public:
	CHacksServer();
	virtual ~CHacksServer();

	virtual void Init();

	virtual void SetNet(void *pNet) { m_pNet = (CNetServer *)pNet; }

	virtual int GetVersion() const { return VERSION_06; }

	virtual int GetRole(int Origin);
	virtual int GetOrigin(int Role);

	virtual int Detect(CNetChunk *pPacket);

	virtual const NETADDR *GetPeerAddress(int PeerID);

	int Detect5(CNetChunk *pPacket);
	int DetectHacks(CNetChunk *pPacket);

	virtual void OnRegisterUpdate(int Nettype);
	virtual bool OnRegisterPacket(CNetChunk *pPacket);
};

IHacks *CreateHacks_Server() { return new CHacksServer(); }

CHacksServer::CHacksServer()
{
	m_pNet = 0;
}

CHacksServer::~CHacksServer()
{
}

void CHacksServer::Init()
{
	dbg_assert((bool)m_pNet, "you have to register the net first");
	CHacks::Init();
	m_pServer = Kernel()->RequestInterface<IServer>();
	m_Register.Init(m_pNet, Kernel()->RequestInterface<IEngineMasterServer>(), Kernel()->RequestInterface<IConsole>());
}

int CHacksServer::GetRole(int Origin)
{
	dbg_assert(0 <= Origin && Origin < NUM_ORIGINS, "invalid origin");
	switch(Origin)
	{
	case ORIGIN_OWN: return ROLE_SERVER;
	case ORIGIN_PEER: return ROLE_CLIENT;
	}
	return 0;
}

int CHacksServer::GetOrigin(int Role)
{
	dbg_assert(0 <= Role && Role < NUM_ROLES, "invalid role");
	switch(Role)
	{
	case ROLE_SERVER: return ORIGIN_OWN;
	case ROLE_CLIENT: return ORIGIN_PEER;
	}
	return 0;
}

const NETADDR *CHacksServer::GetPeerAddress(int PeerID)
{
	dbg_assert(0 <= PeerID && PeerID < MAX_CLIENTS, "invalid pid");
	static NETADDR Addr;
	Addr = m_pNet->ClientAddr(PeerID);
	return &Addr;
}

int CHacksServer::Detect(CNetChunk *pPacket)
{
	int Result;
	Result = Detect5(pPacket);
	if(Result)
		return Result;
	return 0;
}

/*int CHacksServer::OnPacket(CNetChunk *pPacket, int Origin)
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
}*/

int CHacksServer::DetectHacks(CNetChunk *pPacket)
{
	if(pPacket->m_Flags&NETSENDFLAG_CONNLESS)
		return 0;

	int ClientID = pPacket->m_ClientID;
	dbg_assert(0 <= ClientID && ClientID < MAX_CLIENTS, "cid out of range");

	if(!Server()->ClientIngame(ClientID))
	{
		CUnpacker Unpacker;
		Unpacker.Reset(pPacket->m_pData, pPacket->m_DataSize);
		
		const char *pVersion = Unpacker.GetString(CUnpacker::SANITIZE_CC);
		if(str_comp(pVersion, Protocol6::GAME_NETVERSION) != 0)
			// wrong version, let the server deal with it
			return 0;

		Unpacker.GetString(); // password

		const unsigned char *pMagic = Unpacker.GetRaw(sizeof(HACKS_MAGIC));
		int NumProtocols = Unpacker.GetInt();
		if(Unpacker.Error() || mem_comp(pMagic, HACKS_MAGIC, sizeof(HACKS_MAGIC)) != 0)
			return 0;
		for(int i = 0; i < NumProtocols; i++)
		{
			const unsigned char *pMod = Unpacker.GetRaw(MOD_ID_LENGTH);
			int Number = Unpacker.GetInt();
			if(Unpacker.Error())
				return 0;
		}
	}
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

void CHacksServer::OnRegisterUpdate(int Nettype)
{
	m_Register.RegisterUpdate(Nettype);
}

bool CHacksServer::OnRegisterPacket(CNetChunk *pPacket)
{
	return m_Register.RegisterProcessPacket(pPacket);
}
