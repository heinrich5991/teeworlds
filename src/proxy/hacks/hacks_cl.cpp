
#include <engine/client.h>
//#include <engine/console.h>

//#include <engine/shared/network.h>
//#include <engine/shared/packer.h>
//#include <engine/shared/protocol.h>
//#include <engine/shared/snapshot.h>

//#include <game/generated/protocol.h>

//#include <proxy/proxy/0.5/nethash.h>
//#include <proxy/proxy/0.5/protocol.h>
//#include <proxy/proxy/0.5/mastersrv.h>

//#include <proxy/proxy/proxy.h>

#include "hacks.h"

//#include "hacks_register.h"

class CHacksClient : public CHacks
{
private:
	IClient *m_pClient;
	CNetClient *m_pNet;
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

	virtual void OnRegisterUpdate(int Nettype) { dbg_assert(false, "not reachable"); }
	virtual bool OnRegisterPacket(CNetChunk *pPacket) { dbg_assert(false, "not reachable"); return false; }
};

IHacks *CreateHacks_Client() { return new CHacksClient(); }

CHacksClient::CHacksClient()
{
	m_pNet = 0;
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

int CHacksClient::Detect(CNetChunk *pPacket)
{
	return 0;
}

int CHacksClient::DetectHacks(CNetChunk *pPacket)
{
	return 0;
}
