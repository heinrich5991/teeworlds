
#ifndef PROXY_PROXY_H
#define PROXY_PROXY_H

class IHacks;
class CNetChunk;
class CSnapshot;

typedef void (*PACKET_FUNC)(CNetChunk *pPacket, void *pUserData);

enum
{
	VERSION_05=0,
	VERSION_06,
	NUM_VERSIONS,

	ORIGIN_SERVER=0,
	ORIGIN_CLIENT,
	NUM_ORIGINS,
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

	virtual void TranslatePacket(CNetChunk *pChunk, int Origin) = 0;
	virtual int TranslateServerSnap(CSnapshot *pSnap) = 0; // in-place, returns size
};

IProxy *CreateProxy(int ServerVer, int ClientVer, IHacks *pHacks, PACKET_FUNC pfnTranslatedPacketCB, void *pUserData);

#endif // PROXY_PROXY_H

