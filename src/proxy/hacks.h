
#ifndef PROXY_HACKS_H
#define PROXY_HACKS_H

#include <engine/kernel.h>

class CNetChunk;
class CSnapshot;

class IHacks : public IInterface
{
	MACRO_INTERFACE("hacks", 0)
public:
	IHacks() {}
	virtual ~IHacks() {}

	// called on init
	virtual void Init() = 0;

	// called to check if more packets are to be sent
	virtual int GetSendPacket(CNetChunk *pPacket) = 0;

	// called to check if more packets are to be received
	virtual int GetRecvPacket(CNetChunk *pPacket) = 0;

	// called on packet arrival, returns nonzero if packet
	// is to be ignored
	virtual int OnSendPacket(CNetChunk *pPacket) = 0;

	// called before a packet is sent, returns nonzero if
	// the packet must not be sent
	virtual int OnRecvPacket(CNetChunk *pPacket) = 0;

	// called after snap creation, to do post processing on it
	virtual void OnSnap(int ClientID, CSnapshot *pSnap, int *pSnapSize) = 0;

	// called on client disconnect
	virtual void OnDisconnect(int ClientID) = 0;

	virtual void SetNet(void *pNet) = 0;
};

IHacks *CreateHacks();

#endif // PROXY_HACKS_H

