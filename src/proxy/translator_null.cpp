
#include <engine/shared/snapshot.h>

#include "translator.h"

class CTranslatorNull : public ITranslator
{
public:
	CTranslatorNull(IHacks *pHacks, PACKET_FUNC pfnTranslatePacketCB, void *pUserData);
	virtual void TranslatePacket(CNetChunk *pPacket);
	virtual int TranslateSnap(CSnapshot *pSnap);
};

ITranslator *CreateTranslatorNull(IHacks *pHacks, PACKET_FUNC pfnTranslatedPacketCB, void *pUserData)
{
	return new CTranslatorNull(pHacks, pfnTranslatedPacketCB, pUserData);
}

CTranslatorNull::CTranslatorNull(IHacks *pHacks, PACKET_FUNC pfnTranslatePacketCB, void *pUserData)
	: ITranslator(pHacks, pfnTranslatePacketCB, pUserData)
{
}

void CTranslatorNull::TranslatePacket(CNetChunk *pPacket)
{
	TranslatePacketCB(pPacket);
}

int CTranslatorNull::TranslateSnap(CSnapshot *pSnap)
{
	CSnapshotBuilder Builder;
	Builder.Init();

	for(int i = 0; i < pSnap->NumItems(); i++)
	{
		CSnapshotItem *pItem = pSnap->GetItem(i);
		int Type = pItem->Type();
		int ID = pItem->ID();
		int Size = pSnap->GetItemSize(i);

		void *pWrite = Builder.NewItem(Type, ID, Size);
		mem_copy(pWrite, pItem->Data(), Size);
	}

	return Builder.Finish(pSnap);
}

