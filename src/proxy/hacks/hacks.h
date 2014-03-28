#include <engine/server.h>

#include <engine/shared/network.h>
#include <engine/shared/packer.h>
#include <engine/shared/protocol.h>
#include <engine/shared/snapshot.h>

#include <game/generated/protocol.h>

#include <proxy/proxy/proxy.h>

#include "../hacks.h"

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
	};

public:
	CHacks();
	virtual ~CHacks();

	virtual void Init() = 0;
	virtual int GetSendPacket(CNetChunk *pPacket) { return GetPacket(pPacket, ORIGIN_OWN); }
	virtual int GetRecvPacket(CNetChunk *pPacket) { return GetPacket(pPacket, ORIGIN_PEER); }
	virtual int OnSendPacket(CNetChunk *pPacket) { return OnPacket(pPacket, ORIGIN_OWN); }
	virtual int OnRecvPacket(CNetChunk *pPacket) { return OnPacket(pPacket, ORIGIN_PEER); }

	virtual void OnSnap(int PeerID, CSnapshot *pSnap, int *pSnapSize);
	virtual int OnDisconnect(int PeerID);

	virtual int CreateDeltaServer(int PeerID, CSnapshot *pFrom, CSnapshot *pTo, void *pDelta);
	virtual int UnpackDeltaClient(int PeerID, CSnapshot *pFrom, CSnapshot *pTo, void *pDelta, int DeltaSize);
	virtual void *EmptyDeltaClient(int PeerID);

	virtual void PostSnapshotStorageAddClient(int PeerID, CSnapshotStorage *pStorage, CSnapshot *pAltSnap, int AltSize);
	virtual bool OverrideStandardMapCheckClient(int PeerID);

	virtual int GetPacket(CNetChunk *pPacket, int Origin);
	virtual int OnPacket(CNetChunk *pPacket, int Origin);

	virtual void TranslatePacketCBImpl(CNetChunk *pPacket);
	static void TranslatePacketCB(CNetChunk *pPacket, void *pUserData)
	{
		((CHacks *)pUserData)->TranslatePacketCBImpl(pPacket);
	}

	virtual void InitCBData(int Origin, int PeerID);
	virtual void FinalizeCBData(int Origin);

	IProxy *CreateProxy(int PeerVersion);
	void SetProxy(int PeerID, int Version);
	virtual int GetConnlessVersion(CNetChunk *pPacket);
	const char *GetVersionString(int Version);

	virtual const NETADDR *GetPeerAddress(int PeerID) = 0;

	virtual int GetRole(int Origin) = 0;
	virtual int GetOrigin(int Role) = 0;

	virtual int Detect(CNetChunk *pPacket) = 0;

	virtual int GetVersion() const = 0;

protected:
	CSnapshotDelta m_SnapshotDelta;

	enum
	{
		MAX_PACKETS_PER_PACKET=16,
	};

	struct CPeer
	{
		NETADDR m_Addr;
		IProxy *m_pProxy;
	};

	CPeer m_aPeers[MAX_CLIENTS];

	struct CCBData
	{
		bool m_Active;
		int m_PeerID;
		CNetChunk m_aPackets[MAX_PACKETS_PER_PACKET];
		CPacker m_aPacketData[MAX_PACKETS_PER_PACKET];
		int m_NumPackets;
		int m_Offset;
	};

	CCBData m_aCBData[NUM_ORIGINS];
	int m_CBOrigin;

	NETADDR m_ConnlessAddr;
	IProxy *m_pConnlessAddrProxy;

	IProxy *m_apConnlessProxies[NUM_VERSIONS];
};

