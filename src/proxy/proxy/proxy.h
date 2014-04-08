#ifndef PROXY_PROXY_H
#define PROXY_PROXY_H

#include "versions.h"

class CNetChunk;
class CSnapshot;

typedef void (*PACKET_FUNC)(CNetChunk *pPacket, void *pUserData);

struct CProxyCB
{
	PACKET_FUNC m_pfnCallback;
	void *m_pUserdata;
};

class IProxy
{
public:
	IProxy() {}
	virtual ~IProxy() {}

	virtual void TranslateClientPacket(CNetChunk *pChunk, CProxyCB ClientCB, CProxyCB ServerCB) = 0;
	virtual void TranslateServerPacket(CNetChunk *pChunk, CProxyCB ClientCB, CProxyCB ServerCB) = 0;
	virtual int TranslateServerSnap(CSnapshot *pSnap) = 0; // in-place, returns size
};

IProxy *CreateProxy(int ServerVer, int ClientVer);

#endif // PROXY_PROXY_H
