
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

	ROLE_CLIENT=0,
	ROLE_SERVER,
	NUM_ROLES,
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

	virtual void TranslatePacket(CNetChunk *pChunk, int Role) = 0;
	virtual int TranslateServerSnap(CSnapshot *pSnap) = 0; // in-place, returns size

	virtual int CreateDeltaServer(CSnapshot *pFrom, CSnapshot *pTo, void *pData) = 0;
	virtual int UnpackDeltaClient(CSnapshot *pFrom, CSnapshot *pTo, void *pData, int DataSize) = 0;
	virtual void *EmptyDeltaClient() = 0;
};

IProxy *CreateProxy(int ServerVer, int ClientVer, IHacks *pHacks, PACKET_FUNC pfnTranslatedPacketCB, void *pUserData);

#endif // PROXY_PROXY_H

