
#include <engine/server.h>

#include <engine/shared/network.h>
#include <engine/shared/packer.h>
#include <engine/shared/protocol.h>
#include <engine/shared/snapshot.h>

#include <game/generated/protocol.h>

#include "nethash.h"
#include "proxy.h"
#include "protocol.h"
#include "protocol_generated.h"

static const unsigned char SERVERBROWSE_GETINFO_OLD[] = {255, 255, 255, 255, 'g', 'i', 'e', '2'};
static const unsigned char SERVERBROWSE_INFO_OLD[] = {255, 255, 255, 255, 'i', 'n', 'f', '2'};

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
	virtual void PostSnap(int ClientID, CSnapshot *pSnap, int *pSnapSize);

	CPacker m_RecvPacker;
	CPacker m_SendPacker;

	CNetObjHandler m_NetObjHandler;
	Protocol5::CNetObjHandler m_NetObjHandlerOld;

	CSnapshotBuilder m_Builder;
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
			|| Msg == Protocol5::NETMSGTYPE_CL_CHANGEINFO
			|| Msg == Protocol5::NETMSGTYPE_CL_CALLVOTE)
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
		else if(Msg == Protocol5::NETMSGTYPE_CL_CALLVOTE)
		{
			Protocol5::CNetMsg_Cl_CallVote *pData = (Protocol5::CNetMsg_Cl_CallVote *)pRawData;
			CNetMsg_Cl_CallVote NewData;
			NewData.m_Type = pData->m_Type;
			NewData.m_Value = pData->m_Value;
			NewData.m_Reason = "";
			NewData.Pack((CMsgPacker *)&m_RecvPacker);
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

		void *pRawData;
		if(Msg == NETMSGTYPE_SV_VOTEOPTIONLISTADD
			|| Msg == NETMSGTYPE_SV_VOTEOPTIONREMOVE)
		{
			pRawData = m_NetObjHandlerOld.SecureUnpackMsg(Msg, &Unpacker);
			if(!pRawData)
				return 1;
		}

		if(Msg == NETMSGTYPE_SV_VOTEOPTIONLISTADD)
		{
			CNetMsg_Sv_VoteOptionListAdd *pData = (CNetMsg_Sv_VoteOptionListAdd *)pRawData;
			Protocol5::CNetMsg_Sv_VoteOption OldData;
			#define _(x) pData->m_pDescription ## x
			const char *m_apVoteOptions[] =
				{ _( 0), _( 1), _( 2), _( 3), _( 4),
				  _( 5), _( 6), _( 7), _( 8), _( 9),
				  _(10), _(11), _(12), _(13), _(14) };
			#undef _
			const int NUM_VOTE_OPTIONS = sizeof(m_apVoteOptions) / sizeof(m_apVoteOptions[0]);

			for(int i = 0; i < NUM_VOTE_OPTIONS; i++)
			{
				OldData.m_pCommand = m_apVoteOptions[i];
				OldData.Pack((CMsgPacker *)&m_SendPacker);
				// proxy: TODO: implement send
			}

			return 1;
		}
	}

	m_RecvPacker.AddRaw(&Unpacker);
	pPacket->m_DataSize = m_RecvPacker.Size();
	pPacket->m_pData = m_RecvPacker.Data();

	return 0;
}

int CHandler_05endofline::PreProcessConnlessPacket(CNetChunk *pPacket)
{
	if(mem_comp(pPacket->m_pData, SERVERBROWSE_GETINFO_OLD,
		sizeof(SERVERBROWSE_GETINFO_OLD)) == 0)
	{
		return 1; // proxy: TODO: implement server info
	}
	return 0;
}

void CHandler_05endofline::PostSnap(int ClientID, CSnapshot *pSnap, int *pSnapSize)
{
	CSnapshotBuilder Builder;
	Builder.Init();

	Protocol5::CNetObj_Game GameInfo;
	mem_zero(&GameInfo, sizeof(GameInfo));
	bool GotGameInfo = false, GotGameData = false;
	int RedFlagCarriedBy = -1, BlueFlagCarriedBy = -1;
	bool WroteFlagRed = false, WroteFlagBlue = false;

	// first, gather some info
	for(int i = 0; i < pSnap->NumItems(); i++)
	{
		CSnapshotItem *pItem = pSnap->GetItem(i);
		if(pItem->Type() == NETOBJTYPE_GAMEINFO)
		{
			dbg_assert(!GotGameInfo, "two game info in one snap?"); // proxy: TODO: check this before release
			CNetObj_GameInfo *pData = (CNetObj_GameInfo *)pItem->Data();
			GameInfo.m_Flags = pData->m_GameFlags;
			GameInfo.m_RoundStartTick = pData->m_RoundStartTick;
			GameInfo.m_GameOver = (bool) pData->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER;
			GameInfo.m_SuddenDeath = (bool) pData->m_GameStateFlags&GAMESTATEFLAG_SUDDENDEATH;
			GameInfo.m_Paused = (bool) pData->m_GameStateFlags&GAMESTATEFLAG_PAUSED;
			GameInfo.m_ScoreLimit = pData->m_ScoreLimit;
			GameInfo.m_TimeLimit = pData->m_TimeLimit;
			GameInfo.m_Warmup = pData->m_WarmupTimer;
			GameInfo.m_RoundNum = pData->m_RoundNum;
			GameInfo.m_RoundCurrent = pData->m_RoundCurrent;
			GotGameInfo = true;
		}
		else if(pItem->Type() == NETOBJTYPE_GAMEDATA)
		{
			dbg_assert(!GotGameData, "two game data in one snap?");
			CNetObj_GameData *pData = (CNetObj_GameData *)pItem->Data();
			GameInfo.m_TeamscoreRed = pData->m_TeamscoreRed;
			GameInfo.m_TeamscoreBlue = pData->m_TeamscoreBlue;
			RedFlagCarriedBy = pData->m_FlagCarrierRed;
			BlueFlagCarriedBy = pData->m_FlagCarrierBlue;
			GotGameInfo = true;
		}
	}
	dbg_assert(GotGameInfo, "no game info??");

	// second, build new snapshot
	for(int i = 0; i < pSnap->NumItems(); i++)
	{
		CSnapshotItem *pItem = pSnap->GetItem(i);
		int Type = pItem->Type();
		int ID = pItem->ID();
		int Size = pSnap->GetItemSize(i);

		int NewType = Type;
		if(Type >= NETOBJTYPE_CHARACTERCORE)
			NewType = Type - 1;
		if(Type >= NETEVENTTYPE_COMMON)
			NewType = Type - 2;

		if(Type == NETOBJTYPE_GAMEDATA
			|| Type == NETOBJTYPE_SPECTATORINFO)
			continue; // drop these

		if(Type == NETOBJTYPE_CHARACTER)
		{
			CNetObj_Character *pData = (CNetObj_Character *)pItem->Data();

			// abuse that it has the same layout
			Protocol5::CNetObj_Character OldData = *(Protocol5::CNetObj_Character *)pData;

			OldData.m_PlayerState = Protocol5::PLAYERSTATE_UNKNOWN;
			if(pData->m_PlayerFlags&PLAYERFLAG_PLAYING)
				OldData.m_PlayerState = Protocol5::PLAYERSTATE_PLAYING;
			if(pData->m_PlayerFlags&PLAYERFLAG_IN_MENU)
				OldData.m_PlayerState = Protocol5::PLAYERSTATE_IN_MENU;
			if(pData->m_PlayerFlags&PLAYERFLAG_CHATTING)
				OldData.m_PlayerState = Protocol5::PLAYERSTATE_CHATTING;
			void *pWrite = Builder.NewItem(NewType, ID, sizeof(OldData));
			mem_copy(pWrite, &OldData, sizeof(OldData));
		}
		else if(Type == NETOBJTYPE_PLAYERINFO)
		{
			CNetObj_PlayerInfo *pData = (CNetObj_PlayerInfo *)pItem->Data();
			Protocol5::CNetObj_PlayerInfo OldData;
			mem_zero(&OldData, sizeof(OldData));
			mem_copy(&OldData, pData, sizeof(*pData));

			void *pWrite = Builder.NewItem(NewType, ID, sizeof(OldData));
			mem_copy(pWrite, &OldData, sizeof(OldData));
		}
		else if(Type == NETOBJTYPE_FLAG)
		{
			CNetObj_Flag *pData = (CNetObj_Flag *)pItem->Data();
			Protocol5::CNetObj_Flag OldData;

			OldData.m_X = pData->m_X;
			OldData.m_Y = pData->m_Y;
			OldData.m_Team = pData->m_Team;
			if(pData->m_Team == 0)
			{
				dbg_assert(!WroteFlagRed, "two red flags");
				OldData.m_CarriedBy = RedFlagCarriedBy;
				WroteFlagRed = true;
			}
			else if(pData->m_Team == 1)
			{
				dbg_assert(!WroteFlagBlue, "two blue flags");
				OldData.m_CarriedBy = BlueFlagCarriedBy;
				WroteFlagBlue = true;
			}
			else
				OldData.m_CarriedBy = -1;
			if(OldData.m_CarriedBy < 0)
				OldData.m_CarriedBy = -1;
			void *pWrite = Builder.NewItem(NewType, ID, sizeof(OldData));
			mem_copy(pWrite, &OldData, sizeof(OldData));
		}
		else if(Type == NETOBJTYPE_GAMEINFO)
		{
			void *pWrite = Builder.NewItem(Protocol5::NETOBJTYPE_GAME, ID, sizeof(GameInfo));
			mem_copy(pWrite, &GameInfo, sizeof(GameInfo));
		}
		else if(Type == NETOBJTYPE_CLIENTINFO)
		{
			CNetObj_ClientInfo *pData = (CNetObj_ClientInfo *)pItem->Data();
			Protocol5::CNetObj_ClientInfo OldData;

			OldData.m_Name0 = pData->m_Name0; // proxy: TODO: utf-8 parsing?
			OldData.m_Name1 = pData->m_Name1;
			OldData.m_Name2 = pData->m_Name2;
			OldData.m_Name3 = pData->m_Name3;
			OldData.m_Name4 = 0;
			OldData.m_Name5 = 0;
			OldData.m_Skin0 = pData->m_Skin0;
			OldData.m_Skin1 = pData->m_Skin1;
			OldData.m_Skin2 = pData->m_Skin2;
			OldData.m_Skin3 = pData->m_Skin3;
			OldData.m_Skin4 = pData->m_Skin4;
			OldData.m_Skin5 = pData->m_Skin5;
			OldData.m_UseCustomColor = pData->m_UseCustomColor;
			OldData.m_ColorBody = pData->m_ColorBody;
			OldData.m_ColorFeet = pData->m_ColorFeet;

			void *pWrite = Builder.NewItem(NewType, ID, sizeof(OldData));
			mem_copy(pWrite, &OldData, sizeof(OldData));
		}
		else
		{
			void *pWrite = Builder.NewItem(NewType, ID, Size);
			mem_copy(pWrite, pItem->Data(), Size);
		}
	}

	*pSnapSize = Builder.Finish(pSnap);
}

