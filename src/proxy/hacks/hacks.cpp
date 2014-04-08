
#include <proxy/proxy/0.5/mastersrv.h>

#include "hacks.h"

CHacks::CHacks()
{
	mem_zero(m_apConnlessProxies, sizeof(m_apConnlessProxies));
	mem_zero(m_apSnapshotHandlers, sizeof(m_apSnapshotHandlers));
	mem_zero(m_aPeers, sizeof(m_aPeers));
	mem_zero(&m_ConnlessAddr, sizeof(m_ConnlessAddr));

	m_pfnSendFunction = 0;
	m_pSendFunctionUserdata = 0;
	m_pConnlessAddrProxy = 0;

	for(int i = 0; i < NUM_VERSIONS; i++)
		m_apSnapshotHandlers[i] = CreateSnapshotHandler(i);

	CNetObjHandler NetObjHandler;
	for(int i = 0; i < NUM_NETOBJTYPES; i++)
		m_SnapshotDelta.SetStaticsize(i, NetObjHandler.GetObjSize(i));
}

CHacks::~CHacks()
{
	for(int i = 0; i < NUM_VERSIONS; i++)
	{
		if(m_apConnlessProxies[i])
			delete m_apConnlessProxies[i];
		m_apConnlessProxies[i] = 0;
	}
	for(int i = 0; i < NUM_VERSIONS; i++)
	{
		if(m_apSnapshotHandlers[i])
			delete m_apSnapshotHandlers[i];
		m_apSnapshotHandlers[i] = 0;
	}
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(m_aPeers[i].m_pProxy)
			delete m_aPeers[i].m_pProxy;
		m_aPeers[i].m_pProxy = 0;
		// don't delete the snapshot handler,
		// the peer doesn't own it
		m_aPeers[i].m_pSnapshotHandler = 0;
	}

	mem_zero(m_aPeers, sizeof(m_aPeers));
	m_pConnlessAddrProxy = 0;
}

void CHacks::Init()
{
	// this is here instead of the constructor because it
	// would otherwise result in a pure virtual call GetVersion()
	for(int i = 0; i < NUM_VERSIONS; i++)
		m_apConnlessProxies[i] = CreateProxy(i);
}

void CHacks::TranslatePacketSendCBImpl(CNetChunk *pPacket)
{
	m_pfnSendFunction(pPacket, m_pSendFunctionUserdata);
}

void CHacks::TranslatePacketRecvCBImpl(CNetChunk *pPacket)
{
	CRecvData *pRecvData = m_RecvBuffer.Allocate(sizeof(*pRecvData));
	dbg_assert((bool)pRecvData, "too many packets per packet"); // proxy: TODO: check before release

	pRecvData->m_PacketData.Reset();
	pRecvData->m_PacketData.AddRaw(pPacket->m_pData, pPacket->m_DataSize);
	pRecvData->m_Packet = *pPacket;
	pRecvData->m_Packet.m_pData = pRecvData->m_PacketData.Data();
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
				TranslatePacket(m_apConnlessProxies[Version], pPacket, Origin);
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
					TranslatePacket(m_aPeers[PeerID].m_pProxy, pPacket, Origin);
					return 1;
				}
			}
		}

		// connlesshack begin
		if(Origin == ORIGIN_OWN && m_pConnlessAddrProxy
			&& net_addr_comp(&pPacket->m_Address, &m_ConnlessAddr) == 0)
		{
			TranslatePacket(m_pConnlessAddrProxy, pPacket, Origin); // proxy: TODO: fix this
			return 1;
		}
		// connlesshack end

		return 0;
	}

	dbg_assert(0 <= PeerID && PeerID < MAX_CLIENTS, "pid out of range");
	if(m_aPeers[PeerID].m_pProxy)
	{
		TranslatePacket(m_aPeers[PeerID].m_pProxy, pPacket, Origin);
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

void CHacks::SetSendFunction(HACKS_SEND_FUNC pfnSendFunction, void *pUserdata)
{
	m_pfnSendFunction = pfnSendFunction;
	m_pSendFunctionUserdata = pUserdata;
}

int CHacks::GetRecvPacket(CNetChunk *pPacket)
{
	CRecvData *pRecvData = m_RecvBuffer.First();
	if(!pRecvData)
	{
		// connlesshack begin
		// reset the connless addr proxy if all packets have been read
		m_pConnlessAddrProxy = 0;
		// connlesshack end
		return 0;
	}

	m_GetRecvPacketData.Reset();
	m_GetRecvPacketData.AddRaw(pRecvData->m_Packet.m_pData, pRecvData->m_Packet.m_DataSize);

	*pPacket = pRecvData->m_Packet;
	pPacket->m_pData = m_GetRecvPacketData.Data();

	m_RecvBuffer.PopFirst();
	pRecvData = 0;

	// connlesshack begin
	if(pPacket->m_Flags&NETSENDFLAG_CONNLESS)
	{
		m_ConnlessAddr = pPacket->m_Address;
		m_pConnlessAddrProxy = m_apConnlessProxies[VERSION_05];
	}
	else
		m_pConnlessAddrProxy = 0;
	// connlesshack end

	return 1;
}

void CHacks::SetProxy(int PeerID, int Version)
{
	dbg_assert(0 <= PeerID && PeerID < MAX_CLIENTS, "pid out of range");
	dbg_assert(0 <= Version && Version < NUM_VERSIONS, "invalid version");

	if(Version == GetVersion())
	{
		m_aPeers[PeerID].m_pProxy = 0;
		m_aPeers[PeerID].m_pSnapshotHandler = 0;
		return;
	}

	m_aPeers[PeerID].m_pProxy = CreateProxy(Version);
	m_aPeers[PeerID].m_pSnapshotHandler = m_apSnapshotHandlers[Version];
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
		return ::CreateProxy(GetVersion(), PeerVersion);
	if(OwnRole == ROLE_CLIENT)
		return ::CreateProxy(PeerVersion, GetVersion());

	dbg_assert(false, "unreachable");
	return 0;
}

void CHacks::InitCBData(CProxyCB *pClientCB, CProxyCB *pServerCB)
{
	CProxyCB SendCB;
	CProxyCB RecvCB;

	SendCB.m_pfnCallback = TranslatePacketSendCB;
	SendCB.m_pUserdata = this;
	RecvCB.m_pfnCallback = TranslatePacketRecvCB;
	RecvCB.m_pUserdata = this;

	if(GetRole(ORIGIN_OWN) == ROLE_CLIENT)
	{
		// if we're the client, origin client means send,
		// origin server means recv
		*pClientCB = SendCB;
		*pServerCB = RecvCB;
	}
	else
	{
		// vice versa for the server
		*pClientCB = RecvCB;
		*pServerCB = SendCB;
	}
}

void CHacks::TranslatePacket(IProxy *pProxy, CNetChunk *pPacket, int Origin)
{
	dbg_assert((bool)pProxy, "invalid proxy");

	CProxyCB ClientCB;
	CProxyCB ServerCB;
	InitCBData(&ClientCB, &ServerCB);
	if(GetRole(Origin) == ROLE_CLIENT)
		pProxy->TranslateClientPacket(pPacket, ClientCB, ServerCB);
	else
		pProxy->TranslateServerPacket(pPacket, ClientCB, ServerCB);
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
	dbg_assert(GetRole(ORIGIN_OWN) == ROLE_SERVER, "create_delta_server called in client");
	if(m_aPeers[PeerID].m_pSnapshotHandler)
		return m_aPeers[PeerID].m_pSnapshotHandler->CreateDelta(pFrom, pTo, pDelta);
	return m_SnapshotDelta.CreateDelta(pFrom, pTo, pDelta);
}

int CHacks::UnpackDeltaClient(int PeerID, CSnapshot *pFrom, CSnapshot *pTo, void *pDelta, int DeltaSize)
{
	dbg_assert(GetRole(ORIGIN_OWN) == ROLE_CLIENT, "unpack_delta_client called in server");
	if(m_aPeers[PeerID].m_pSnapshotHandler)
		return m_aPeers[PeerID].m_pSnapshotHandler->UnpackDelta(pFrom, pTo, pDelta, DeltaSize);
	return m_SnapshotDelta.UnpackDelta(pFrom, pTo, pDelta, DeltaSize);
}

void *CHacks::EmptyDeltaClient(int PeerID)
{
	dbg_assert(GetRole(ORIGIN_OWN) == ROLE_CLIENT, "unpack_delta_client called in server");
	if(m_aPeers[PeerID].m_pSnapshotHandler)
		return m_aPeers[PeerID].m_pSnapshotHandler->EmptyDelta();
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

bool CHacks::OverrideStandardMapCheckClient(int PeerID)
{
	return (bool)m_aPeers[PeerID].m_pProxy;
}
