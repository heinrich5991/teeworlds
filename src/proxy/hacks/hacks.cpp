
#include <proxy/proxy/0.5/mastersrv.h>

#include "hacks.h"

CHacks::CHacks()
{
	mem_zero(m_aPeers, sizeof(m_aPeers));
	mem_zero(m_aCBData, sizeof(m_aCBData));
	mem_zero(&m_ConnlessAddr, sizeof(m_ConnlessAddr));
	m_pConnlessAddrProxy = 0;
	m_CBOrigin = -1;

}

CHacks::~CHacks()
{
	mem_zero(m_apConnlessProxies, sizeof(m_apConnlessProxies));
	mem_zero(m_aPeers, sizeof(m_aPeers));
	m_pConnlessAddrProxy = 0;
}

void CHacks::Init()
{
	// this is here instead of the constructor because it
	// would otherwise result in a pure virtual call GetVersion()
	for(int i = 0; i < NUM_VERSIONS; i++)
		m_apConnlessProxies[i] = CreateProxy(i);

	CNetObjHandler NetObjHandler;
	for(int i = 0; i < NUM_NETOBJTYPES; i++)
		m_SnapshotDelta.SetStaticsize(i, NetObjHandler.GetObjSize(i));
}

void CHacks::InitCBData(int Origin, int PeerID)
{
	dbg_assert(0 <= Origin && Origin < NUM_ORIGINS, "invalid origin");
	dbg_assert(!m_aCBData[Origin].m_Active, "must not be active at that time");
	dbg_assert(m_aCBData[Origin].m_NumPackets == 0, "cannot init, not empty");

	//mem_zero(m_aCBData[Origin], sizeof(m_aCBData[Origin]));
	m_aCBData[Origin].m_PeerID = PeerID;
	m_aCBData[Origin].m_Offset = 0;
	m_aCBData[Origin].m_Active = true;

	m_CBOrigin = Origin;
}

void CHacks::FinalizeCBData(int Origin)
{
	dbg_assert(m_aCBData[Origin].m_Active, "must be active at that time");
	m_aCBData[Origin].m_Active = false;
}

void CHacks::TranslatePacketCBImpl(CNetChunk *pPacket)
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

int CHacks::OnPacket(CNetChunk *pPacket, int Origin)
{
	int PeerID = pPacket->m_ClientID;
	dbg_assert(-1 <= PeerID && PeerID < MAX_CLIENTS, "pid out of range");

	if(pPacket->m_Flags&NETSENDFLAG_CONNLESS)
	{
		// connless packet to one of our peers?
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(m_aPeers[i].m_pProxy
				&& net_addr_comp(&m_aPeers[i].m_Addr, &pPacket->m_Address) == 0)
			{
				InitCBData(Origin, i);
				m_aPeers[i].m_pProxy->TranslatePacket(pPacket, GetRole(Origin));
				FinalizeCBData(Origin);
				return 1;
			}
		}

		// connless packet is response to another connless packet?
		if(Origin == ORIGIN_OWN && m_pConnlessAddrProxy
			&& net_addr_comp(&pPacket->m_Address, &m_ConnlessAddr) == 0)
		{
			InitCBData(Origin, PeerID);
			m_pConnlessAddrProxy->TranslatePacket(pPacket, GetRole(Origin)); // proxy: TODO: fix this
			FinalizeCBData(Origin);
			return 1;
		}

		// try to guess the version of the connless packet
		int Version = GetConnlessVersion(pPacket);
		if(m_apConnlessProxies[Version])
		{
			InitCBData(Origin, PeerID);
			m_apConnlessProxies[Version]->TranslatePacket(pPacket, GetRole(Origin));
			FinalizeCBData(Origin);
			return 1;
		}

		return 0;
	}

	dbg_assert(0 <= PeerID && PeerID < MAX_CLIENTS, "pid out of range");
	if(m_aPeers[PeerID].m_pProxy)
	{
		InitCBData(Origin, PeerID);
		m_aPeers[PeerID].m_pProxy->TranslatePacket(pPacket, GetRole(Origin));
		FinalizeCBData(Origin);
		return 1;
	}

	if(Origin == ORIGIN_PEER)
	{
		int Result = Detect(pPacket);
		if(Result)
			return Result;
	}

	return 0;
}

int CHacks::GetConnlessVersion(CNetChunk *pPacket)
{
	#define COMP(x) (mem_comp(pPacket->m_pData, (x), sizeof(x)) == 0)
	#define COMP5(x) COMP(Protocol5::x)
	// 0.5 begin
	if(pPacket->m_DataSize >= (int)sizeof(Protocol5::SERVERBROWSE_GETINFO))
	{
		if(COMP5(SERVERBROWSE_HEARTBEAT)
			|| COMP5(SERVERBROWSE_HEARTBEAT)
			|| COMP5(SERVERBROWSE_GETLIST)
			|| COMP5(SERVERBROWSE_LIST)
			|| COMP5(SERVERBROWSE_GETCOUNT)
			|| COMP5(SERVERBROWSE_COUNT)
			|| COMP5(SERVERBROWSE_GETINFO)
			|| COMP5(SERVERBROWSE_INFO)
			|| COMP5(SERVERBROWSE_OLD_GETINFO)
			|| COMP5(SERVERBROWSE_OLD_INFO))
			// same as 0.6:
			// || COMP5(SERVERBROWSE_FWCHECK)
			// || COMP5(SERVERBROWSE_FWRESPONSE)
			// || COMP5(SERVERBROWSE_FWOK)
			// || COMP5(SERVERBROWSE_FWERROR)
			return VERSION_05;
	}
	// 0.5 end

	return GetVersion();
	#undef COMP5
	#undef COMP
}


int CHacks::GetPacket(CNetChunk *pPacket, int Origin)
{
	dbg_assert(!m_aCBData[Origin].m_Active, "must be inactive at that time");

	if(m_aCBData[Origin].m_NumPackets == 0)
		return 0;

	*pPacket = m_aCBData[Origin].m_aPackets[m_aCBData[Origin].m_Offset];
	m_aCBData[Origin].m_Offset++;
	m_aCBData[Origin].m_NumPackets--;

	if(Origin == ORIGIN_PEER)
	{
		if(pPacket->m_Flags&NETSENDFLAG_CONNLESS)
		{
			m_ConnlessAddr = pPacket->m_Address;
			m_pConnlessAddrProxy = m_apConnlessProxies[VERSION_05];
		}
		else
			m_pConnlessAddrProxy = 0;
	}

	return 1;
}

void CHacks::SetProxy(int PeerID, int Version)
{
	dbg_assert(0 <= PeerID && PeerID < MAX_CLIENTS, "pid out of range");
	dbg_assert(0 <= Version && Version < NUM_VERSIONS, "invalid version");

	if(Version == GetVersion())
	{
		m_aPeers[PeerID].m_pProxy = 0;
		return;
	}

	m_aPeers[PeerID].m_pProxy = CreateProxy(Version);
	m_aPeers[PeerID].m_Addr = *GetPeerAddress(PeerID);

	char aAddress[NETADDR_MAXSTRSIZE];
	net_addr_str(&m_aPeers[PeerID].m_Addr, aAddress, sizeof(aAddress));
	dbg_msg("proxy", "version detected pid=%d addr=%s ver=%s", PeerID, aAddress, GetVersionString(Version));
}

IProxy *CHacks::CreateProxy(int PeerVersion)
{
	dbg_assert(0 <= PeerVersion && PeerVersion < NUM_VERSIONS, "invalid version");

	// no need for proxy if the peer has the same version as ourselves
	if(PeerVersion == GetVersion())
		return 0;

	int OwnRole = GetRole(ORIGIN_OWN);
	if(OwnRole == ROLE_SERVER)
		return ::CreateProxy(GetVersion(), PeerVersion, this, TranslatePacketCB, this);
	if(OwnRole == ROLE_CLIENT)
		return ::CreateProxy(PeerVersion, GetVersion(), this, TranslatePacketCB, this);
	return 0;
}

const char *CHacks::GetVersionString(int Version)
{
	dbg_assert(0 <= Version && Version < NUM_VERSIONS, "invalid version");
	switch(Version)
	{
	case VERSION_05: return "0.5";
	case VERSION_06: return "0.6";
	}
	return "";
}

void CHacks::OnSnap(int PeerID, CSnapshot *pSnap, int *pSnapSize)
{
	dbg_assert(0 <= PeerID && PeerID < MAX_CLIENTS, "pid out of range"); // proxy: TODO: check that before release
	if(m_aPeers[PeerID].m_pProxy)
	{
		int SnapSize = m_aPeers[PeerID].m_pProxy->TranslateServerSnap(pSnap);
		if(pSnapSize)
			*pSnapSize = SnapSize;
	}
}

int CHacks::OnDisconnect(int PeerID)
{
	if(m_aPeers[PeerID].m_pProxy)
	{
		dbg_msg("proxy", "proxy removed pid=%d", PeerID);
		delete m_aPeers[PeerID].m_pProxy;
	}
	mem_zero(&m_aPeers[PeerID], sizeof(m_aPeers[PeerID]));
	return 0;
}

int CHacks::CreateDeltaServer(int PeerID, CSnapshot *pFrom, CSnapshot *pTo, void *pDelta)
{
	if(m_aPeers[PeerID].m_pProxy)
		return m_aPeers[PeerID].m_pProxy->CreateDeltaServer(pFrom, pTo, pDelta);
	return m_SnapshotDelta.CreateDelta(pFrom, pTo, pDelta);
}

int CHacks::UnpackDeltaClient(int PeerID, CSnapshot *pFrom, CSnapshot *pTo, void *pDelta, int DeltaSize)
{
	if(m_aPeers[PeerID].m_pProxy)
		return m_aPeers[PeerID].m_pProxy->UnpackDeltaClient(pFrom, pTo, pDelta, DeltaSize);
	return m_SnapshotDelta.UnpackDelta(pFrom, pTo, pDelta, DeltaSize);
}

void *CHacks::EmptyDeltaClient(int PeerID)
{
	if(m_aPeers[PeerID].m_pProxy)
		return m_aPeers[PeerID].m_pProxy->EmptyDeltaClient();
	return m_SnapshotDelta.EmptyDelta();
}

