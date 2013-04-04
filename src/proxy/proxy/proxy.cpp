
#include <base/system.h>

#include "translator.h"
#include "translator_null.h"
#include "0.5-0.6/translator.h"

#include "proxy.h"

class CProxyTranslator : public IProxy
{
private:
	ITranslator *m_apTranslators[NUM_ROLES];
public:
	CProxyTranslator(ITranslator *pServerT, ITranslator *pClientT)
		: IProxy(0, 0, 0)
	{
		m_apTranslators[ROLE_SERVER] = pServerT;
		m_apTranslators[ROLE_CLIENT] = pClientT;
	}

	~CProxyTranslator()
	{
		delete m_apTranslators[ROLE_CLIENT]; // proxy: TODO: come up with nice deallocation idea
		delete m_apTranslators[ROLE_SERVER];
		mem_zero(m_apTranslators, sizeof(m_apTranslators));
	}

	virtual void TranslatePacket(CNetChunk *pChunk, int Origin)
	{
		m_apTranslators[Origin]->TranslatePacket(pChunk);
	}

	virtual int TranslateServerSnap(CSnapshot *pSnap)
	{
		return m_apTranslators[ROLE_SERVER]->TranslateSnap(pSnap);
	}
};

IProxy *CreateProxy(int ServerVer, int ClientVer, IHacks *pHacks, PACKET_FUNC pfnTranslatedPacketCB, void *pUserData)
{
	dbg_assert(0 <= ServerVer && ServerVer < NUM_VERSIONS, "invalid server version");
	dbg_assert(0 <= ClientVer && ClientVer < NUM_VERSIONS, "invalid client version");

	ITranslator *pServerT = 0;
	ITranslator *pClientT = 0;

	if(ClientVer == ServerVer)
	{
		pServerT = CreateTranslatorNull(pHacks, pfnTranslatedPacketCB, pUserData);
		pClientT = CreateTranslatorNull(pHacks, pfnTranslatedPacketCB, pUserData);
	}
	else if(ClientVer == VERSION_05 && ServerVer == VERSION_06)
	{
		pServerT = CreateTranslator_06_05(pHacks, pfnTranslatedPacketCB, pUserData);
		pClientT = CreateTranslator_05_06(pHacks, pfnTranslatedPacketCB, pUserData);
	}
	else if(ClientVer == VERSION_06 && ServerVer == VERSION_05)
	{
		pServerT = CreateTranslator_05_06(pHacks, pfnTranslatedPacketCB, pUserData);
		pClientT = CreateTranslator_06_05(pHacks, pfnTranslatedPacketCB, pUserData);
	}
	else
		dbg_assert(false, "invalid version pair");

	return new CProxyTranslator(pServerT, pClientT);
}

