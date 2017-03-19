
#include <engine/client.h>

//#include <engine/shared/network.h>
//#include <engine/shared/packer.h>
//#include <engine/shared/protocol.h>
//#include <engine/shared/snapshot.h>

//#include <game/generated/protocol.h>

#include <mastersrv/mastersrv.h>

#include <proxy/proxy/0.5/nethash.h>
#include <proxy/proxy/0.6/nethash.h>

//#include <proxy/proxy/proxy.h>

#include "hacks.h"

//#include "hacks_register.h"

class CHacksClient : public CHacks
{
private:
	IClient *m_pClient;
	CNetClient *m_pNet;
	int m_InDisconnect;

	struct CBrowserAddress
	{
		NETADDR m_Address;
		int m_Version;
		CBrowserAddress *m_pNext;
	};

	CBrowserAddress *m_apBrowserAddresses[256];
	CHeap m_BrowserAddressHeap;
public:
	IClient *Client() { return m_pClient; }

public:
	CHacksClient();
	virtual ~CHacksClient();

	virtual void Init();

	virtual void SetNet(void *pNet) { m_pNet = (CNetClient *)pNet; }

	virtual int GetVersion() const { return VERSION_06; }

	virtual int GetRole(int Origin);
	virtual int GetOrigin(int Role);

	virtual int Detect(CNetChunk *pPacket);

	virtual const NETADDR *GetPeerAddress(int PeerID);

	int Detect5(CNetChunk *pPacket);
	int DetectHacks(CNetChunk *pPacket);

	virtual int OnPacket(CNetChunk *pPacket, int Origin);

	virtual int OnDisconnect(int PeerID);

	virtual void OnRegisterUpdate(int Nettype) { dbg_assert(false, "not reachable"); }
	virtual bool OnRegisterPacket(CNetChunk *pPacket) { dbg_assert(false, "not reachable"); return false; }
};

IHacks *CreateHacks_Client() { return new CHacksClient(); }

CHacksClient::CHacksClient()
{
	m_pNet = 0;
	m_InDisconnect = 0;
	mem_zero(m_aNumBrowserAddresses, sizeof(m_aNumBrowserAddresses));
}

CHacksClient::~CHacksClient()
{
}

void CHacksClient::Init()
{
	dbg_assert((bool)m_pNet, "you have to register the net first");
	CHacks::Init();
	m_pClient = Kernel()->RequestInterface<IClient>();
}

int CHacksClient::GetRole(int Origin)
{
	dbg_assert(0 <= Origin && Origin < NUM_ORIGINS, "invalid origin");
	switch(Origin)
	{
	case ORIGIN_OWN: return ROLE_CLIENT;
	case ORIGIN_PEER: return ROLE_SERVER;
	}
	return 0;
}

int CHacksClient::GetOrigin(int Role)
{
	dbg_assert(0 <= Role && Role < NUM_ROLES, "invalid role");
	switch(Role)
	{
	case ROLE_SERVER: return ORIGIN_PEER;
	case ROLE_CLIENT: return ORIGIN_OWN;
	}
	return 0;
}

const NETADDR *CHacksClient::GetPeerAddress(int PeerID)
{
	dbg_assert(0 <= PeerID && PeerID < 1, "invalid pid");
	static NETADDR Addr;
	Addr = m_pNet->ServerAddr();
	return &Addr;
}

int CHacksClient::OnDisconnect(int PeerID)
{
	dbg_assert(0 <= PeerID && PeerID < 1, "invalid pid");

	if(m_InDisconnect)
		return 0;

	if(m_aPeers[PeerID].m_pProxy != 0 || Client()->State() != IClient::STATE_LOADING)
	{
		return CHacks::OnDisconnect(PeerID);
	}

	const char *pError = m_pNet->ErrorString();
	if(!pError)
		return CHacks::OnDisconnect(PeerID);

	dbg_msg("dbg", "error '%s'", pError);

	int ErrLength = str_length(pError) + 1;
	const char *pNetHash = 0;

	int Version = VERSION_06;
	for(int i = 0; i < ErrLength; i++)
	{
		bool IsHex = ('0' <= pError[i] && pError[i] <= '9')
			|| ('a' <= pError[i] && pError[i] <= 'f');
		if(pNetHash && !IsHex)
		{
			// if the nethash has the right size
			if(&pError[i] - pNetHash == NETHASH_LENGTH)
			{
				if(str_comp_num(pNetHash, Protocol6::GAME_NETVERSION_HASH,
					NETHASH_LENGTH) == 0)
					; // just do nothing
				else if(str_comp_num(pNetHash, Protocol5::GAME_NETVERSION_HASH,
					NETHASH_LENGTH) == 0)
				{
					if(Version != -1)
						Version = VERSION_05; // version 5 detected
				}
				// a nethash different from 0.5 and 0.6
				else
					Version = -1;
			}
			pNetHash = 0;
			
		}
		else if(!pNetHash && IsHex)
			pNetHash = &pError[i];
	}

	// is it useful to employ a proxy?
	if(Version != VERSION_06 && Version != -1)
	{
		NETADDR Addr = *GetPeerAddress(PeerID);

		m_InDisconnect = 1;
		m_pNet->Disconnect(0);
		m_InDisconnect = 0;

		m_pNet->Connect(&Addr);
		Client()->SetState(IClient::STATE_CONNECTING);
		SetProxy(PeerID, Version);
		return 1;
	}
	return CHacks::OnDisconnect(PeerID);
}

int CHacksClient::OnPacket(CNetChunk *pPacket, int Origin)
{
	if(Origin == ORIGIN_OWN && pPacket->m_Flags&NETSENDFLAG_CONNLESS)
	{
		if(pPacket->m_DataSize >= (int)sizeof(SERVERBROWSE_GETLIST)
			&& mem_comp(pPacket->m_pData, SERVERBROWSE_GETLIST, sizeof(SERVERBROWSE_GETLIST)) == 0)
		{
			mem_zero(m_apBrowserAddresses, sizeof(m_apBrowserAddresses));
			m_BrowserAddressHeap.Reset();

			for(int i = 0; i < NUM_VERSIONS; i++)
			{
				InitCBData(Origin, pPacket->m_ClientID);
				m_apConnlessProxies[i]->TranslatePacket(pPacket, GetRole(Origin));
				FinalizeCBData(Origin);
			}
			return 1;
		}
		else if(pPacket->m_DataSize >= (int)sizeof(SERVERBROWSE_GETINFO)
			&& mem_comp(pPacket->m_pData, SERVERBROWSE_GETINFO, sizeof(SERVERBROWSE_GETINFO)) == 0)
		{
			for(int i = 0; i < NUM_VERSIONS; i++)
			{
				if(i == GetVersion())
					continue;
				for(int k = 0; k < m_aNumBrowserAddresses[i]; k++)
				{
					if(net_addr_comp(&m_aaBrowserAddresses[i][k], &pPacket->m_Address) == 0)
					{
						InitCBData(Origin, pPacket->m_ClientID);
						m_apConnlessProxies[i]->TranslatePacket(pPacket, GetRole(Origin));
						FinalizeCBData(Origin);
						return 1;
					}
				}
			}
		}
	}
	else if(Origin == ORIGIN_PEER && pPacket->m_Flags&NETSENDFLAG_CONNLESS)
	{
		int Result = CHacks::OnPacket(pPacket, Origin);
		if(pPacket->m_DataSize >= (int)sizeof(SERVERBROWSE_LIST)
			&& mem_comp(pPacket->m_pData, SERVERBROWSE_LIST, sizeof(SERVERBROWSE_LIST)) == 0)
		{
			m_BrowserAddressHeap.Allocate(sizeof());

			for(int i = 0; i < NUM_VERSIONS; i++)
			{
				InitCBData(Origin, pPacket->m_ClientID);
				m_apConnlessProxies[i]->TranslatePacket(pPacket, GetRole(Origin));
				FinalizeCBData(Origin);
			}
			return 1;
		}
	}
	return CHacks::OnPacket(pPacket, Origin);
}

int CHacksClient::Detect(CNetChunk *pPacket)
{
	int Result;
	Result = Detect5(pPacket);
	if(Result)
		return Result;
	return 0;
}

int CHacksClient::Detect5(CNetChunk *pPacket)
{
	return 0;
}

int CHacksClient::DetectHacks(CNetChunk *pPacket)
{
	return 0;
}

