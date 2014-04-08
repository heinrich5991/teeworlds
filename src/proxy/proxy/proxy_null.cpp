
#include "proxy_null.h"

#include "proxy.h"
#include "proxy_macros.h"

#include <engine/shared/snapshot.h>

class CProxyNull : public IProxy
{
public:
	CProxyNull();
	virtual void TranslateClientPacket(CNetChunk *pPacket, CProxyCB ClientCB, CProxyCB ServerCB);
	virtual void TranslateServerPacket(CNetChunk *pPacket, CProxyCB ClientCB, CProxyCB ServerCB);
	virtual int TranslateServerSnap(CSnapshot *pSnap); // in-place, returns size
};

IProxy *CreateProxyNull()
{
	return new CProxyNull();
}

CProxyNull::CProxyNull()
	: IProxy()
{
}

void CProxyNull::TranslateClientPacket(CNetChunk *pPacket, CProxyCB ClientCB, CProxyCB ServerCB)
{
	PROXY_CLIENT_PACKET(pPacket);
}

void CProxyNull::TranslateServerPacket(CNetChunk *pPacket, CProxyCB ClientCB, CProxyCB ServerCB)
{
	PROXY_SERVER_PACKET(pPacket);
}

int CProxyNull::TranslateServerSnap(CSnapshot *pSnap)
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
