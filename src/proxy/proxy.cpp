
// #includes for engine/server/server.h
//	#include <engine/shared/console.h>
//	#include <engine/shared/protocol.h>
#include <engine/server.h>

#include <engine/shared/network.h>
#include <engine/shared/packer.h>
#include <engine/shared/protocol.h>

#include "0.5-endofline/src/nethash.h"
#include "0.5-endofline/src/network.h"
#include "0.5-endofline/src/protocol.h"
#include "0.5-endofline/src/proxy.h"

#include "proxy.h"

class CProxy : public IHacks
{
private:
	IServer *m_pServer;
public:
	IServer *Server() { return m_pServer; }

public:
	CProxy();
	virtual ~CProxy();
	virtual void Init();
	virtual int PreProcessClientPacket(CNetChunk *pPacket);
	virtual int PreSendClientPacket(CNetChunk *pPacket);
	virtual int PreProcessConnlessPacket(CNetChunk *pPacket);

private:
	enum
	{
		HANDLER_05ENDOFLINE=0,
		NUM_HANDLERS,
	};
	IHacks *m_apHandlers[NUM_HANDLERS];
	IHacks *m_apClientHandlers[MAX_CLIENTS];

};

IHacks *CreateHacks() { return new CProxy(); }

CProxy::CProxy()
{
	m_apHandlers[HANDLER_05ENDOFLINE] = CreateHandler_05endofline(&m_pKernel);
	mem_zero(m_apClientHandlers, sizeof(m_apClientHandlers));
}

CProxy::~CProxy()
{
	for(int i = 0; i < NUM_HANDLERS; i++)
	{
		delete m_apHandlers[i];
		m_apHandlers[i] = 0;
	}
}

void CProxy::Init()
{
	for(int i = 0; i < NUM_HANDLERS; i++)
		m_apHandlers[i]->Init();
	m_pServer = Kernel()->RequestInterface<IServer>();
}

int CProxy::PreProcessClientPacket(CNetChunk *pPacket)
{
	int ClientID = pPacket->m_ClientID;
	dbg_assert(0 <= ClientID && ClientID < MAX_CLIENTS, "cid out of range");
	if(m_apClientHandlers[ClientID])
	{
		return m_apClientHandlers[ClientID]->PreProcessClientPacket(pPacket);
	}

	// 0.5-endofline begin
	if(!Server()->ClientIngame(ClientID))
	//if(Server()->m_aClients[ClientID].m_State == IServer::CClient::STATE_AUTH)
	{
		CUnpacker Unpacker;
		Unpacker.Reset(pPacket->m_pData, pPacket->m_DataSize);

		int Msg = Unpacker.GetInt();
		int Sys = Msg&1;
		Msg >>= 1;

		if(Unpacker.Error())
		{
			dbg_msg("proxy", "shouldn't happen"); // proxy: TODO: remove this
			return false;
		}

		if(Sys && Msg == Protocol5::NETMSG_INFO)
		{
			const char *pVersion = Unpacker.GetString(CUnpacker::SANITIZE_CC);
			if(str_comp(pVersion, Protocol5::GAME_NETVERSION) == 0)
			{
				m_apClientHandlers[ClientID] = m_apHandlers[HANDLER_05ENDOFLINE];
				dbg_msg("proxy", "0.5 detected cid=%x", ClientID);
				return PreProcessClientPacket(pPacket);
			}
		}
	}
	// 0.5-endofline end

	return false;
}

int CProxy::PreSendClientPacket(CNetChunk *pPacket)
{
	int ClientID = pPacket->m_ClientID;
	dbg_assert(0 <= ClientID && ClientID < MAX_CLIENTS, "cid out of range");
	if(m_apClientHandlers[ClientID])
	{
		return m_apClientHandlers[ClientID]->PreSendClientPacket(pPacket);
	}
	return 0;
}

int CProxy::PreProcessConnlessPacket(CNetChunk *pPacket)
{
	for(int i = 0; i < NUM_HANDLERS; i++)
	{
		int Result = m_apHandlers[i]->PreProcessConnlessPacket(pPacket);
		if(Result != 0)
			return Result;
	}
	return 0;
}

