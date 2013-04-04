
#ifndef PROXY_TRANSLATOR_H
#define PROXY_TRANSLATOR_H

#include "proxy.h"

class ITranslator
{
private:
	IHacks *m_pHacks;
public:
	IHacks *Hacks() { return m_pHacks; }

private:
	PACKET_FUNC m_pfnTranslatedPacketCB; // callback
	void *m_pUserData;

public:
	ITranslator(IHacks *pHacks, PACKET_FUNC pfnTranslatedPacketCB, void *pUserData)
	{
		m_pHacks = pHacks;

		m_pfnTranslatedPacketCB = pfnTranslatedPacketCB;
		m_pUserData = pUserData;
	}
	virtual ~ITranslator() {}
	virtual void TranslatePacketCB(CNetChunk *pPacket)
	{
		(*m_pfnTranslatedPacketCB)(pPacket, m_pUserData);
	}

	virtual void TranslatePacket(CNetChunk *pPacket) = 0;
	virtual int TranslateSnap(CSnapshot *pSnap) = 0; // in-place, returns size
};

#endif // PROXY_TRANSLATOR_H

