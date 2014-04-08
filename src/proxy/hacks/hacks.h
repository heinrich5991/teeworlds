#include <engine/server.h>

#include <engine/shared/network.h>
#include <engine/shared/packer.h>
#include <engine/shared/protocol.h>
#include <engine/shared/snapshot.h>

#include <game/generated/protocol.h>

#include <proxy/proxy/proxy.h>
#include <proxy/proxy/snapshot_handler.h>

#include <proxy/hacks.h>

static const unsigned char HACKS_MAGIC[] = { 0x40, 0x49, 0x0f, 0xdb };

enum
{
	NETHASH_LENGTH=16,
	MOD_ID_LENGTH=8,
};

class CHacks : public IHacks
{
protected:
	enum
	{
		ORIGIN_OWN=0,
		ORIGIN_PEER,
		NUM_ORIGINS,

		ROLE_CLIENT=0,
		ROLE_SERVER,
		NUM_ROLES,
	};

public:
	CHacks();
	virtual ~CHacks();

	virtual void Init() = 0;

	virtual void SetSendFunction(HACKS_SEND_FUNC pfnSendFunction, void *pUserdata);
	virtual int GetRecvPacket(CNetChunk *pPacket);

	virtual int OnSendPacket(CNetChunk *pPacket) { return OnPacket(pPacket, ORIGIN_OWN); }
	virtual int OnRecvPacket(CNetChunk *pPacket) { return OnPacket(pPacket, ORIGIN_PEER); }

	virtual void OnSnap(int PeerID, CSnapshot *pSnap, int *pSnapSize);
	virtual int OnDisconnect(int PeerID);

	virtual int CreateDeltaServer(int PeerID, CSnapshot *pFrom, CSnapshot *pTo, void *pDelta);
	virtual int UnpackDeltaClient(int PeerID, CSnapshot *pFrom, CSnapshot *pTo, void *pDelta, int DeltaSize);
	virtual void *EmptyDeltaClient(int PeerID);

	virtual void PostSnapshotStorageAddClient(int PeerID, CSnapshotStorage *pStorage, CSnapshot *pAltSnap, int AltSize);
	virtual bool OverrideStandardMapCheckClient(int PeerID);

	virtual int OnPacket(CNetChunk *pPacket, int Origin);

	virtual void TranslatePacketSendCBImpl(CNetChunk *pPacket);
	static void TranslatePacketSendCB(CNetChunk *pPacket, void *pUserdata)
	{
		((CHacks *)pUserdata)->TranslatePacketSendCBImpl(pPacket);
	}

	virtual void TranslatePacketRecvCBImpl(CNetChunk *pPacket);
	static void TranslatePacketRecvCB(CNetChunk *pPacket, void *pUserdata)
	{
		((CHacks *)pUserdata)->TranslatePacketRecvCBImpl(pPacket);
	}

	// six helper functions
	IProxy *CreateProxy(int PeerVersion);
	void InitCBData(CProxyCB *ClientCB, CProxyCB *ServerCB);
	void TranslatePacket(IProxy *pProxy, CNetChunk *pPacket, int Origin);

	void SetProxy(int PeerID, int Version);
	virtual int GetConnlessVersion(CNetChunk *pPacket);
	const char *GetVersionString(int Version);

	virtual const NETADDR *GetPeerAddress(int PeerID) = 0;

	virtual int GetRole(int Origin) = 0;
	virtual int GetOrigin(int Role) = 0;

	virtual int Detect(CNetChunk *pPacket) = 0;

	virtual int GetVersion() const = 0;

protected:
	HACKS_SEND_FUNC m_pfnSendFunction;
	void *m_pSendFunctionUserdata;

	CSnapshotDelta m_SnapshotDelta;

	ISnapshotHandler *m_apSnapshotHandlers[NUM_VERSIONS];

	enum
	{
		MAX_PACKETS_PER_PACKET=16,
	};

	struct CPeer
	{
		NETADDR m_Addr;
		IProxy *m_pProxy;
		ISnapshotHandler *m_pSnapshotHandler;
	};

	CPeer m_aPeers[MAX_CLIENTS];

	struct CRecvData
	{
		CNetChunk m_aPackets[MAX_PACKETS_PER_PACKET];
		CPacker m_aPacketData[MAX_PACKETS_PER_PACKET];
		int m_NumPackets;
		int m_Offset;
	};

	CRecvData m_RecvData;

	NETADDR m_ConnlessAddr;
	IProxy *m_pConnlessAddrProxy;

	IProxy *m_apConnlessProxies[NUM_VERSIONS];
};

