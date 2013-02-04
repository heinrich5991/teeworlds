
#include <engine/server.h>

#include <engine/shared/network.h>
#include <engine/shared/packer.h>
#include <engine/shared/protocol.h>

#include <game/generated/protocol.h>

#include "nethash.h"
#include "proxy.h"
#include "protocol.h"
#include "protocol_generated.h"

class CHandler_05endofline : public IHacks
{
private:
	IKernel **m_ppKernel;
	IServer *m_pServer;
	IGameServer *m_pGameServer;
public:
	IServer *Server() { return m_pServer; }
	IGameServer *GameServer() { return m_pGameServer; }

public:
	CHandler_05endofline(IKernel **ppKernel);
	virtual void Init();
	virtual int PreProcessClientPacket(CNetChunk *pPacket);
	virtual int PreSendClientPacket(CNetChunk *pPacket);
	virtual int PreProcessConnlessPacket(CNetChunk *pPacket);

	CPacker m_RecvPacker;
	CPacker m_SendPacker;

	CNetObjHandler m_NetObjHandler;
	Protocol5::CNetObjHandler m_NetObjHandlerOld;
};

IHacks *CreateHandler_05endofline(IKernel **ppKernel) { return new CHandler_05endofline(ppKernel); }

CHandler_05endofline::CHandler_05endofline(IKernel **ppKernel)
{
	m_ppKernel = ppKernel; // proxy: TODO: this is a hack!
}

void CHandler_05endofline::Init()
{
	m_pKernel = *m_ppKernel;
	m_pServer = Kernel()->RequestInterface<IServer>();
	m_pGameServer = Kernel()->RequestInterface<IGameServer>();
}

int CHandler_05endofline::PreProcessClientPacket(CNetChunk *pPacket)
{
	CUnpacker Unpacker;
	Unpacker.Reset(pPacket->m_pData, pPacket->m_DataSize);
	m_RecvPacker.Reset();

	int Msg = Unpacker.GetInt();
	int Sys = Msg&1;
	Msg >>= 1;

	if(Unpacker.Error())
		return 0;

	if(Sys)
	{
		int NewMsg = Msg;
		if(Msg >= Protocol5::NETMSG_SNAP)
			// a new msg 'NETMSG_CONREADY' was added in between the other messages
			NewMsg = Msg + 1;

		m_RecvPacker.AddInt(NewMsg << 1 | Sys);

		if(NewMsg == Protocol5::NETMSG_INFO)
		{
			const char *pVersion = Unpacker.GetString(CUnpacker::SANITIZE_CC);
			if(str_comp(pVersion, Protocol5::GAME_NETVERSION) == 0)
				m_RecvPacker.AddString(GameServer()->NetVersion(), 0);
			else
				m_RecvPacker.AddString(pVersion, 0);
		}

	}
	else
	{
		int NewMsg = Msg;
		if(Msg == Protocol5::NETMSGTYPE_SV_VOTEOPTION)
			NewMsg = NETMSGTYPE_SV_VOTEOPTIONADD;
		if (Msg >= Protocol5::NETMSGTYPE_SV_VOTESET)
			NewMsg = Msg + 2;
		if (Msg >= Protocol5::NETMSGTYPE_CL_STARTINFO)
			NewMsg = Msg + 3;

		m_RecvPacker.AddInt(NewMsg << 1 | Sys);

		void *pRawData;
		if(Msg == Protocol5::NETMSGTYPE_CL_STARTINFO
			|| Msg == Protocol5::NETMSGTYPE_CL_CHANGEINFO)
		{
			pRawData = m_NetObjHandlerOld.SecureUnpackMsg(Msg, &Unpacker);
			if(!pRawData)
				return 1;
		}

		if(Msg == Protocol5::NETMSGTYPE_CL_STARTINFO
			|| Msg == Protocol5::NETMSGTYPE_CL_CHANGEINFO)
		{
			Protocol5::CNetMsg_Cl_StartInfo *pData = (Protocol5::CNetMsg_Cl_StartInfo *)pRawData;
			CNetMsg_Cl_StartInfo NewData;
			NewData.m_pName = pData->m_pName;
			NewData.m_pClan = "";
			NewData.m_Country = -1;
			NewData.m_pSkin = pData->m_pSkin;
			NewData.m_UseCustomColor = pData->m_UseCustomColor;
			NewData.m_ColorBody = pData->m_ColorBody;
			NewData.m_ColorFeet = pData->m_ColorFeet;
			NewData.Pack((CMsgPacker *)&m_RecvPacker); // proxy: TODO: hack
		}
	}

	m_RecvPacker.AddRaw(&Unpacker);
	pPacket->m_DataSize = m_RecvPacker.Size();
	pPacket->m_pData = m_RecvPacker.Data();

	return 0;
}

int CHandler_05endofline::PreSendClientPacket(CNetChunk *pPacket)
{
	CUnpacker Unpacker;
	Unpacker.Reset(pPacket->m_pData, pPacket->m_DataSize);
	m_RecvPacker.Reset();

	int Msg = Unpacker.GetInt();
	int Sys = Msg&1;
	Msg >>= 1;

	if(Unpacker.Error())
		return 0;

	if(Sys)
	{
		int OldMsg = Msg;

		if(Msg == NETMSG_CON_READY)
			// this message is new
			return 1;

		if(Msg >= NETMSG_SNAP)
			// the new msg 'NETMSG_CONREADY' was added
			OldMsg = Msg - 1;

		m_RecvPacker.AddInt(OldMsg << 1 | Sys);
	}
	else
	{
		int OldMsg = Msg;

		if(Msg == NETMSGTYPE_SV_VOTEOPTIONLISTADD
			|| Msg == NETMSGTYPE_SV_VOTEOPTIONREMOVE // proxy: TODO: add suport here
			|| Msg == NETMSGTYPE_CL_SETSPECTATORMODE)
			return 1;

		if(Msg == NETMSGTYPE_SV_VOTEOPTIONADD)
			OldMsg = Protocol5::NETMSGTYPE_SV_VOTEOPTION;
		if (Msg >= NETMSGTYPE_SV_VOTESET)
			OldMsg = Msg - 2;
		if (Msg >= NETMSGTYPE_CL_STARTINFO)
			OldMsg = Msg - 3;

		m_RecvPacker.AddInt(OldMsg << 1 | Sys);
	}

	m_RecvPacker.AddRaw(&Unpacker);
	pPacket->m_DataSize = m_RecvPacker.Size();
	pPacket->m_pData = m_RecvPacker.Data();

	return 0;
}

int CHandler_05endofline::PreProcessConnlessPacket(CNetChunk *pPacket)
{
	return 0;
}

