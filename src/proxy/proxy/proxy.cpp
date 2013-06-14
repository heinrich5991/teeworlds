
#include <base/system.h>
#include <engine/shared/snapshot.h>

#include "translator.h"
#include "translator_null.h"

#include "0.5-0.6/translator.h"

	// for protocol_generated.h
	#include <engine/message.h>
#include "0.5/protocol_generated.h"
#include "0.6/protocol_generated.h"

#include "proxy.h"

class CProxyTranslator : public IProxy
{
private:
	ITranslator *m_apTranslators[NUM_ROLES];
	CSnapshotDelta *m_apSnapshotDeltas[NUM_ROLES];
public:
	CProxyTranslator(ITranslator *pServerT, ITranslator *pClientT,
		CSnapshotDelta *pServerD, CSnapshotDelta *pClientD)
		: IProxy(0, 0, 0)
	{
		m_apTranslators[ROLE_SERVER] = pServerT;
		m_apTranslators[ROLE_CLIENT] = pClientT;
		m_apSnapshotDeltas[ROLE_SERVER] = pServerD;
		m_apSnapshotDeltas[ROLE_CLIENT] = pClientD;
	}

	~CProxyTranslator()
	{
		// TODO: come up with a better deallocation strategy
		for(int i = 0; i < NUM_ROLES; i++)
		{
			delete m_apTranslators[i];
			//delete m_apSnapshotDeltas[i];
		}
		mem_zero(m_apTranslators, sizeof(m_apTranslators));
		mem_zero(m_apSnapshotDeltas, sizeof(m_apSnapshotDeltas));
	}

	virtual void TranslatePacket(CNetChunk *pChunk, int Origin)
	{
		m_apTranslators[Origin]->TranslatePacket(pChunk);
	}

	virtual int TranslateServerSnap(CSnapshot *pSnap)
	{
		return m_apTranslators[ROLE_SERVER]->TranslateSnap(pSnap);
	}

	virtual void *EmptyDeltaClient()
	{
		return 0;
	}

	virtual int CreateDeltaServer(CSnapshot *pFrom, CSnapshot *pTo, void *pData)
	{
		return m_apSnapshotDeltas[ROLE_SERVER]->CreateDelta(pFrom, pTo, pData);
	}

	virtual int UnpackDeltaClient(CSnapshot *pFrom, CSnapshot *pTo, void *pData, int DataSize)
	{
		if(pData == 0)
		{
			pData = m_apSnapshotDeltas[ROLE_CLIENT]->EmptyDelta();
			DataSize = sizeof(int) * 3;
		}
		return m_apSnapshotDeltas[ROLE_CLIENT]->UnpackDelta(pFrom, pTo, pData, DataSize);
	}
};

IProxy *CreateProxy(int ServerVer, int ClientVer, IHacks *pHacks, PACKET_FUNC pfnTranslatedPacketCB, void *pUserData)
{
	dbg_assert(0 <= ServerVer && ServerVer < NUM_VERSIONS, "invalid server version");
	dbg_assert(0 <= ClientVer && ClientVer < NUM_VERSIONS, "invalid client version");

	static CSnapshotDelta *s_pSnapshotDelta[NUM_VERSIONS] = { 0 };

	// already initialised
	if(!s_pSnapshotDelta[0])
	{
		for(int i = 0; i < NUM_VERSIONS; i++)
		{
			CSnapshotDelta *pSD = 0;
			switch(i)
			{
			case VERSION_05:
			case VERSION_06:
				pSD = new CSnapshotDelta(); 
			}
			switch(i)
			{
			case VERSION_05:
				{
					Protocol5::CNetObjHandler NOH;
					for(int i = 0; i < Protocol5::NUM_NETOBJTYPES; i++)
						pSD->SetStaticsize(i, NOH.GetObjSize(i));
					break;
				}
			case VERSION_06:
				{
					Protocol6::CNetObjHandler NOH;
					for(int i = 0; i < Protocol6::NUM_NETOBJTYPES; i++)
						pSD->SetStaticsize(i, NOH.GetObjSize(i));
					break;
				}
			}
			dbg_assert((bool)pSD, "not all versions covered");
			s_pSnapshotDelta[i] = pSD;
		}
	}

	ITranslator *pServerT = 0;
	ITranslator *pClientT = 0;
	CSnapshotDelta *pServerD = 0;
	CSnapshotDelta *pClientD = 0;

	pServerD = s_pSnapshotDelta[ClientVer];
	pClientD = s_pSnapshotDelta[ServerVer];

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

	if(!pServerT || !pClientT || !pServerD || !pClientD)
		dbg_assert(false, "invalid version pair");

	return new CProxyTranslator(pServerT, pClientT, pServerD, pClientD);
}

