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

	
};

typedef void (*PACKET_FUNC)(CNetChunk *pPacket, void *pUserData);

enum
{
	VERSION_05=0,
	VERSION_06,
	NUM_VERSIONS
};

class IProxy
{
private:
	IHacks *m_pHacks;
public:
	IHacks *Hacks() { return m_pHacks; }

private:
	PACKET_FUNC m_pfnTranslatedPacketCB; // callback
	void *m_pUserData;

public:
	IProxy(IHacks *pHacks, PACKET_FUNC pfnTranslatedPacketCB, void *pUserData)
	{
		m_pHacks = pHacks;

		m_pfnTranslatedPacketCB = pfnTranslatedPacketCB;
		m_pUserData = pUserData;
	}
	virtual ~IProxy() {}
	virtual void TranslatePacketCB(CNetChunk *pPacket)
	{
		(*m_pfnTranslatedPacketCB)(pPacket, m_pUserData);
	}

	virtual void TranslatePacket(CNetChunk *pPacket) = 0;
	virtual int TranslateSnap(CSnapshot *pSnap) = 0; // in-place, returns size
};

IHacks *CreateHacks();

#endif // PROXY_PROXY_H
