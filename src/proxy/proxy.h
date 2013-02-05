#ifndef PROXY_PROXY_H
#define PROXY_PROXY_H

#include <engine/kernel.h>

class CNetChunk;
class CSnapshot;

class IHacks : public IInterface
{
	MACRO_INTERFACE("hacks", 0)
public:
	IHacks() {}
	virtual ~IHacks() {}
	virtual void Init() = 0;
	virtual int PreProcessClientPacket(CNetChunk *pPacket) = 0;
	virtual int PreSendClientPacket(CNetChunk *pPacket) = 0;
	virtual int PreProcessConnlessPacket(CNetChunk *pPacket) = 0;
	virtual void PostSnap(int ClientID, CSnapshot *pSnap, int *pSnapSize) = 0;
	virtual void PostDisconnect(int ClientID) { }
};

IHacks *CreateHacks();

#endif // PROXY_PROXY_H
