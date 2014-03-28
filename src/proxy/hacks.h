
#ifndef PROXY_HACKS_H
#define PROXY_HACKS_H

#include <engine/kernel.h>

class CNetChunk;
class CSnapshot;
class CSnapshotStorage;

class IHacks : public IInterface
{
	MACRO_INTERFACE("hacks", 0)
public:
	IHacks() {}
	virtual ~IHacks() {}

	// called on init
	virtual void Init() = 0;

	// called to check if more packets are to be sent
	// returns nonzero if there is a packet
	virtual int GetSendPacket(CNetChunk *pPacket) = 0;

	// called to check if more packets are to be received
	// returns nonzero if there is a packet
	virtual int GetRecvPacket(CNetChunk *pPacket) = 0;

	// called on packet arrival, returns nonzero if packet
	// is to be ignored
	virtual int OnSendPacket(CNetChunk *pPacket) = 0;

	// called before a packet is sent, returns nonzero if
	// the packet must not be sent
	virtual int OnRecvPacket(CNetChunk *pPacket) = 0;

	// called after snap creation, to do post processing on it
	virtual void OnSnap(int ClientID, CSnapshot *pSnap, int *pSnapSize) = 0;

	// called on peer disconnect, returns nonzero if no
	// action is to be taken
	virtual int OnDisconnect(int PeerID) = 0;

	// called on register update time
	virtual void OnRegisterUpdate(int Nettype) = 0;
	// called to process register related packets
	virtual bool OnRegisterPacket(CNetChunk *pPacket) = 0;

	// intended to replace the snapshot diff functions
	virtual int CreateDeltaServer(int PeerID, CSnapshot *pFrom, CSnapshot *pTo, void *pDelta) = 0;
	virtual int UnpackDeltaClient(int PeerID, CSnapshot *pFrom, CSnapshot *pTo, void *pDelta, int DeltaSize) = 0;
	virtual void *EmptyDeltaClient(int PeerID) = 0; // returns pointer to CSnapshotDelta::CData

	// intended to be called after the CSnapshotStorage::Add call on the client
	virtual void PostSnapshotStorageAddClient(int PeerID, CSnapshotStorage *pStorage, CSnapshot *pAltSnap, int AltSize) = 0;

	// intended to be called after the standard map check,
	// returns true if the result is to be ignored
	virtual bool OverrideStandardMapCheckClient(int PeerID) = 0;

	virtual void SetNet(void *pNet) = 0;
};

IHacks *CreateHacks_Server();
IHacks *CreateHacks_Client();

#endif // PROXY_HACKS_H

