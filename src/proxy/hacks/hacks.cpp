
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
		// connlesshack begin
		if(Origin == ORIGIN_PEER)
		{
			int Version = GetConnlessVersion(pPacket);
			if(m_apConnlessProxies[Version])
			{
				InitCBData(Origin, PeerID);
				m_apConnlessProxies[Version]->TranslatePacket(pPacket, GetRole(Origin));
				FinalizeCBData(Origin);
				return 1;
			}
			else
				return 0;
		}
		// connlesshack end

		if(Origin == ORIGIN_OWN)
		{
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				if(m_aPeers[PeerID].m_pProxy
					&& net_addr_comp(&m_aPeers[PeerID].m_Addr, &pPacket->m_Address) == 0)
				{
					InitCBData(Origin, PeerID);
					m_aPeers[PeerID].m_pProxy->TranslatePacket(pPacket, GetRole(Origin));
					FinalizeCBData(Origin);
					return 1;
				}
			}
		}

		// connlesshack begin
		if(Origin == ORIGIN_OWN && m_pConnlessAddrProxy
			&& net_addr_comp(&pPacket->m_Address, &m_ConnlessAddr) == 0)
		{
			InitCBData(Origin, PeerID);
			m_pConnlessAddrProxy->TranslatePacket(pPacket, GetRole(Origin)); // proxy: TODO: fix this
			FinalizeCBData(Origin);
			return 1;
		}
		// connlesshack end

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
	// 0.5 begin
	if((unsigned)pPacket->m_DataSize >= sizeof(Protocol5::SERVERBROWSE_GETINFO))
	{
		// abuse that all packets have the same minlength
		if(mem_comp(pPacket->m_pData, Protocol5::SERVERBROWSE_GETINFO,
			    sizeof(Protocol5::SERVERBROWSE_GETINFO)) == 0)
			return VERSION_05;
		if(mem_comp(pPacket->m_pData, Protocol5::SERVERBROWSE_OLD_GETINFO,
			    sizeof(Protocol5::SERVERBROWSE_OLD_GETINFO)) == 0)
			return VERSION_05;
		if(mem_comp(pPacket->m_pData, Protocol5::SERVERBROWSE_INFO,
			    sizeof(Protocol5::SERVERBROWSE_INFO)) == 0)
			return VERSION_05;
		if(mem_comp(pPacket->m_pData, Protocol5::SERVERBROWSE_OLD_INFO,
			    sizeof(Protocol5::SERVERBROWSE_OLD_INFO)) == 0)
			return VERSION_05;
		if(mem_comp(pPacket->m_pData, Protocol5::SERVERBROWSE_LIST,
			    sizeof(Protocol5::SERVERBROWSE_OLD_INFO)) == 0)
			return VERSION_05;
	}
	// 0.5 end

	return VERSION_06;
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

	dbg_msg("proxy", "version detected pid=%d ver=%s", PeerID, GetVersionString(Version));
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
		*pSnapSize = m_aPeers[PeerID].m_pProxy->TranslateServerSnap(pSnap);
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

void CHacks::PostSnapshotStorageAddClient(int PeerID, CSnapshotStorage *pStorage, CSnapshot *pAltSnap, int AltSize)
{
	if(m_aPeers[PeerID].m_pProxy)
	{
		CSnapshotStorage::CHolder *pHolder = pStorage->m_pLast;

		if(!pHolder->m_pAltSnap)
			return;

		int Size = 0;
		Size += sizeof(CSnapshotStorage::CHolder);
		Size += pHolder->m_SnapSize;
		Size += AltSize;

		CSnapshotStorage::CHolder *pNewHolder = (CSnapshotStorage::CHolder *)mem_alloc(Size, 1);

		mem_copy(pNewHolder, pHolder, sizeof(CSnapshotStorage::CHolder));

		// fix pointers
		pNewHolder->m_pSnap = (CSnapshot*)(&pNewHolder[1]);
		pNewHolder->m_pAltSnap = (CSnapshot*)(((char *)pNewHolder->m_pSnap) + pNewHolder->m_SnapSize);
		if(pNewHolder->m_pPrev)
			pNewHolder->m_pPrev->m_pNext = pNewHolder;
		if(pStorage->m_pFirst == pHolder)
			pStorage->m_pFirst = pNewHolder;
		pStorage->m_pLast = pNewHolder;

		mem_copy(pNewHolder->m_pSnap, pHolder->m_pSnap, pNewHolder->m_SnapSize);
		mem_copy(pNewHolder->m_pAltSnap, pAltSnap, AltSize);

		mem_free(pHolder);
		pHolder = 0;
	}
	return;
}
