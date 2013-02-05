
#include <engine/server.h>

#include <engine/shared/network.h>
#include <engine/shared/packer.h>
#include <engine/shared/snapshot.h>

#include <proxy/0.5/nethash.h>
#include <proxy/0.5/protocol.h>
#include <proxy/0.5/protocol_generated.h>

#include <proxy/0.6/nethash.h>
#include <proxy/0.6/protocol.h>
#include <proxy/0.6/protocol_generated.h>

#include "proxy.h"

class CProxy_05_06 : public IProxy
{
public:
	CProxy_05_06(IHacks *pHacks, PACKET_FUNC pfnTranslatePacketCB, void *pUserData);
	virtual void TranslatePacket(CNetChunk *pPacket);
	virtual int TranslateSnap(CSnapshot *pSnap);
};

class CProxy_06_05 : public IProxy
{
public:
	CProxy_06_05(IHacks *pHacks, PACKET_FUNC pfnTranslatePacketCB, void *pUserData);
	virtual void TranslatePacket(CNetChunk *pPacket);
	virtual int TranslateSnap(CSnapshot *pSnap);
};

IProxy *CreateProxy_05_06(IHacks *pHacks, PACKET_FUNC pfnTranslatePacketCB, void *pUserData)
{
	return new CProxy_05_06(pHacks, pfnTranslatePacketCB, pUserData);
}

IProxy *CreateProxy_06_05(IHacks *pHacks, PACKET_FUNC pfnTranslatePacketCB, void *pUserData)
{
	return new CProxy_06_05(pHacks, pfnTranslatePacketCB, pUserData);
}

CProxy_05_06::CProxy_05_06(IHacks *pHacks, PACKET_FUNC pfnTranslatePacketCB, void *pUserData)
	: IProxy(pHacks, pfnTranslatePacketCB, pUserData)
{
}

CProxy_06_05::CProxy_06_05(IHacks *pHacks, PACKET_FUNC pfnTranslatePacketCB, void *pUserData)
	: IProxy(pHacks, pfnTranslatePacketCB, pUserData)
{
}

void CProxy_05_06::TranslatePacket(CNetChunk *pPacket)
{
	if(pPacket->m_Flags&NETSENDFLAG_CONNLESS)
		return; // proxy: TODO: add connless packet handling

	CUnpacker Unpacker;
	Unpacker.Reset(pPacket->m_pData, pPacket->m_DataSize);
	CPacker Packer;
	Packer.Reset();

	int Msg = Unpacker.GetInt();
	int Sys = Msg&1;
	Msg >>= 1;

	if(Unpacker.Error())
		return;

	if(Sys)
	{
		int MsgT = Msg;
		if(Msg >= Protocol5::NETMSG_SNAP)
			// a new msg 'NETMSG_CONREADY' was added in between the other messages
			MsgT = Msg + 1;

		Packer.AddInt(MsgT << 1 | Sys);

		if(MsgT == Protocol5::NETMSG_INFO)
		{
			const char *pVersion = Unpacker.GetString(CUnpacker::SANITIZE_CC);
			if(str_comp(pVersion, Protocol5::GAME_NETVERSION) == 0)
				Packer.AddString(Protocol6::GAME_NETVERSION, 0);
			else
				Packer.AddString(pVersion, 0);
		}
	}
	else
	{
		int MsgT = Msg;
		if(Msg == Protocol5::NETMSGTYPE_SV_VOTEOPTION)
			MsgT = Protocol6::NETMSGTYPE_SV_VOTEOPTIONADD;
		if (Msg >= Protocol5::NETMSGTYPE_SV_VOTESET)
			MsgT = Msg + 2;
		if (Msg >= Protocol5::NETMSGTYPE_CL_STARTINFO)
			MsgT = Msg + 3;

		Packer.AddInt(MsgT << 1 | Sys);

		void *pRawData;
		Protocol5::CNetObjHandler Handler;
		if(Msg == Protocol5::NETMSGTYPE_CL_STARTINFO
			|| Msg == Protocol5::NETMSGTYPE_CL_CHANGEINFO
			|| Msg == Protocol5::NETMSGTYPE_CL_EMOTICON
			|| Msg == Protocol5::NETMSGTYPE_CL_CALLVOTE)
		{
			pRawData = Handler.SecureUnpackMsg(Msg, &Unpacker);
			if(!pRawData)
				return;
		}

		if(Msg == Protocol5::NETMSGTYPE_CL_STARTINFO
			|| Msg == Protocol5::NETMSGTYPE_CL_CHANGEINFO)
		{
			Protocol5::CNetMsg_Cl_StartInfo *pData = (Protocol5::CNetMsg_Cl_StartInfo *)pRawData;
			Protocol6::CNetMsg_Cl_StartInfo DataT;
			DataT.m_pName = pData->m_pName;
			DataT.m_pClan = "";
			DataT.m_Country = -1;
			DataT.m_pSkin = pData->m_pSkin;
			DataT.m_UseCustomColor = pData->m_UseCustomColor;
			DataT.m_ColorBody = pData->m_ColorBody;
			DataT.m_ColorFeet = pData->m_ColorFeet;
			DataT.Pack((CMsgPacker *)&Packer); // proxy: TODO: hack
		}
		else if(Msg == Protocol5::NETMSGTYPE_CL_EMOTICON)
		{
			Protocol5::CNetMsg_Cl_Emoticon *pData = (Protocol5::CNetMsg_Cl_Emoticon *)pRawData;
			Protocol6::CNetMsg_Cl_Emoticon DataT = *(Protocol6::CNetMsg_Cl_Emoticon *)pData;
			// abuse that the emoticon values are nearly the same
			// use Protocol6 variants because they have names
			if(pData->m_Emoticon == Protocol6::EMOTICON_SORRY) // emoticon: music without bubble
				DataT.m_Emoticon = Protocol6::EMOTICON_MUSIC;
			else if(pData->m_Emoticon == Protocol6::EMOTICON_EYES) // emoticon: dead tee
				DataT.m_Emoticon = Protocol6::EMOTICON_SPLATTEE;
			else if(pData->m_Emoticon == Protocol6::EMOTICON_WTF) // no emoticon
				DataT.m_Emoticon = -1;
			else if(pData->m_Emoticon == Protocol6::EMOTICON_QUESTION) // no emoticon
				DataT.m_Emoticon = -1;
			if(DataT.m_Emoticon == -1)
				return;
			DataT.Pack((CMsgPacker *)&Packer);
		}
		else if(Msg == Protocol5::NETMSGTYPE_CL_CALLVOTE)
		{
			Protocol5::CNetMsg_Cl_CallVote *pData = (Protocol5::CNetMsg_Cl_CallVote *)pRawData;
			Protocol6::CNetMsg_Cl_CallVote DataT;
			DataT.m_Type = pData->m_Type;
			DataT.m_Value = pData->m_Value;
			DataT.m_Reason = "";
			DataT.Pack((CMsgPacker *)&Packer);
		}
	}

	Packer.AddRaw(&Unpacker);
	CNetChunk Packet = *pPacket;
	Packet.m_DataSize = Packer.Size();
	Packet.m_pData = Packer.Data();
	TranslatePacketCB(&Packet);
}

void CProxy_06_05::TranslatePacket(CNetChunk *pPacket)
{
	if(pPacket->m_Flags&NETSENDFLAG_CONNLESS)
		return; // proxy: TODO: add connless packet handling

	CUnpacker Unpacker;
	Unpacker.Reset(pPacket->m_pData, pPacket->m_DataSize);
	CPacker Packer;
	Packer.Reset();

	int Msg = Unpacker.GetInt();
	int Sys = Msg&1;
	Msg >>= 1;

	if(Unpacker.Error())
		return;

	if(Sys)
	{
		int MsgT = Msg;

		if(Msg == Protocol6::NETMSG_CON_READY)
			// this message is new
			return;

		if(Msg >= Protocol6::NETMSG_SNAP)
			// the new msg 'NETMSG_CONREADY' was added
			MsgT = Msg - 1;

		Packer.AddInt(MsgT << 1 | Sys);
	}
	else
	{
		int MsgT = Msg;

		if(Msg == Protocol6::NETMSGTYPE_SV_VOTEOPTIONREMOVE // proxy: TODO: add suport here
			|| Msg == Protocol6::NETMSGTYPE_CL_SETSPECTATORMODE)
			return;

		if(Msg == Protocol6::NETMSGTYPE_SV_VOTEOPTIONADD)
			MsgT = Protocol5::NETMSGTYPE_SV_VOTEOPTION;
		if (Msg >= Protocol6::NETMSGTYPE_SV_VOTESET)
			MsgT = Msg - 2;
		if (Msg >= Protocol6::NETMSGTYPE_CL_STARTINFO)
			MsgT = Msg - 3;

		Packer.AddInt(MsgT << 1 | Sys);

		void *pRawData;
		if(Msg == Protocol6::NETMSGTYPE_SV_EMOTICON
			|| Msg == Protocol6::NETMSGTYPE_SV_VOTEOPTIONLISTADD
			|| Msg == Protocol6::NETMSGTYPE_SV_VOTEOPTIONREMOVE)
		{
			Protocol6::CNetObjHandler Handler;
			pRawData = Handler.SecureUnpackMsg(Msg, &Unpacker);
			if(!pRawData)
				return;
		}

		if(Msg == Protocol6::NETMSGTYPE_SV_EMOTICON)
		{
			Protocol6::CNetMsg_Sv_Emoticon *pData = (Protocol6::CNetMsg_Sv_Emoticon *)pRawData;
			Protocol5::CNetMsg_Sv_Emoticon DataT = *(Protocol5::CNetMsg_Sv_Emoticon *)pData;
			// abuse that the emoticon values are nearly the same
			// use Protocol6 variants because they have names
			if(pData->m_Emoticon == Protocol6::EMOTICON_SORRY)
				DataT.m_Emoticon = Protocol6::EMOTICON_OOP;
			else if(pData->m_Emoticon == Protocol6::EMOTICON_WTF)
				DataT.m_Emoticon = Protocol6::EMOTICON_ZZZ; // proxy: TODO: debatable
			else if(pData->m_Emoticon == Protocol6::EMOTICON_EYES)
				DataT.m_Emoticon = Protocol6::EMOTICON_MUSIC;
			else if(pData->m_Emoticon == Protocol6::EMOTICON_QUESTION)
				DataT.m_Emoticon = Protocol6::EMOTICON_DOTDOT;
			DataT.Pack((CMsgPacker *)&Packer);
		}
		else if(Msg == Protocol6::NETMSGTYPE_SV_VOTEOPTIONLISTADD)
		{
			Protocol6::CNetMsg_Sv_VoteOptionListAdd *pData = (Protocol6::CNetMsg_Sv_VoteOptionListAdd *)pRawData;
			Protocol5::CNetMsg_Sv_VoteOption DataT;
			#define _(x) pData->m_pDescription ## x
			const char *m_apVoteOptions[] =
				{ _( 0), _( 1), _( 2), _( 3), _( 4),
				  _( 5), _( 6), _( 7), _( 8), _( 9),
				  _(10), _(11), _(12), _(13), _(14) };
			#undef _
			const int NUM_VOTE_OPTIONS = sizeof(m_apVoteOptions) / sizeof(m_apVoteOptions[0]);

			CNetChunk Packet = *pPacket;
			for(int i = 0; i < NUM_VOTE_OPTIONS; i++)
			{
				DataT.m_pCommand = m_apVoteOptions[i];

				DataT.Pack((CMsgPacker *)&Packer);
				Packet.m_DataSize = Packer.Size();
				Packet.m_pData = Packer.Data();
				TranslatePacketCB(&Packet);
			}

			return;
		}
	}

	Packer.AddRaw(&Unpacker);
	CNetChunk Packet = *pPacket;
	Packet.m_DataSize = Packer.Size();
	Packet.m_pData = Packer.Data();
	TranslatePacketCB(&Packet);
}

int CProxy_05_06::TranslateSnap(CSnapshot *pSnap)
{
	dbg_assert(false, "not implemented yet");
	return 0;
}

int CProxy_06_05::TranslateSnap(CSnapshot *pSnap)
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
		if(pItem->Type() == Protocol6::NETOBJTYPE_GAMEINFO)
		{
			dbg_assert(!GotGameInfo, "two game info in one snap?"); // proxy: TODO: check this before release
			Protocol6::CNetObj_GameInfo *pData = (Protocol6::CNetObj_GameInfo *)pItem->Data();
			GameInfo.m_Flags = pData->m_GameFlags;
			GameInfo.m_RoundStartTick = pData->m_RoundStartTick;
			GameInfo.m_GameOver = (bool) pData->m_GameStateFlags&Protocol6::GAMESTATEFLAG_GAMEOVER;
			GameInfo.m_SuddenDeath = (bool) pData->m_GameStateFlags&Protocol6::GAMESTATEFLAG_SUDDENDEATH;
			GameInfo.m_Paused = (bool) pData->m_GameStateFlags&Protocol6::GAMESTATEFLAG_PAUSED;
			GameInfo.m_ScoreLimit = pData->m_ScoreLimit;
			GameInfo.m_TimeLimit = pData->m_TimeLimit;
			GameInfo.m_Warmup = pData->m_WarmupTimer;
			GameInfo.m_RoundNum = pData->m_RoundNum;
			GameInfo.m_RoundCurrent = pData->m_RoundCurrent;
			GotGameInfo = true;
		}
		else if(pItem->Type() == Protocol6::NETOBJTYPE_GAMEDATA)
		{
			dbg_assert(!GotGameData, "two game data in one snap?");
			Protocol6::CNetObj_GameData *pData = (Protocol6::CNetObj_GameData *)pItem->Data();
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
		if(Type >= Protocol6::NETOBJTYPE_CHARACTERCORE)
			NewType = Type - 1;
		if(Type >= Protocol6::NETEVENTTYPE_COMMON)
			NewType = Type - 2;

		if(Type == Protocol6::NETOBJTYPE_GAMEDATA
			|| Type == Protocol6::NETOBJTYPE_SPECTATORINFO)
			continue; // drop these

		if(Type == Protocol6::NETOBJTYPE_CHARACTER)
		{
			Protocol6::CNetObj_Character *pData = (Protocol6::CNetObj_Character *)pItem->Data();

			// abuse that it has the same layout
			Protocol5::CNetObj_Character DataT = *(Protocol5::CNetObj_Character *)pData;

			DataT.m_PlayerState = Protocol5::PLAYERSTATE_UNKNOWN;
			if(pData->m_PlayerFlags&Protocol6::PLAYERFLAG_PLAYING)
				DataT.m_PlayerState = Protocol5::PLAYERSTATE_PLAYING;
			if(pData->m_PlayerFlags&Protocol6::PLAYERFLAG_IN_MENU)
				DataT.m_PlayerState = Protocol5::PLAYERSTATE_IN_MENU;
			if(pData->m_PlayerFlags&Protocol6::PLAYERFLAG_CHATTING)
				DataT.m_PlayerState = Protocol5::PLAYERSTATE_CHATTING;
			void *pWrite = Builder.NewItem(NewType, ID, sizeof(DataT));
			mem_copy(pWrite, &DataT, sizeof(DataT));
		}
		else if(Type == Protocol6::NETOBJTYPE_PLAYERINFO)
		{
			Protocol6::CNetObj_PlayerInfo *pData = (Protocol6::CNetObj_PlayerInfo *)pItem->Data();
			Protocol5::CNetObj_PlayerInfo DataT;
			mem_zero(&DataT, sizeof(DataT));
			mem_copy(&DataT, pData, sizeof(*pData));

			void *pWrite = Builder.NewItem(NewType, ID, sizeof(DataT));
			mem_copy(pWrite, &DataT, sizeof(DataT));
		}
		else if(Type == Protocol6::NETOBJTYPE_FLAG)
		{
			Protocol6::CNetObj_Flag *pData = (Protocol6::CNetObj_Flag *)pItem->Data();
			Protocol5::CNetObj_Flag DataT;

			DataT.m_X = pData->m_X;
			DataT.m_Y = pData->m_Y;
			DataT.m_Team = pData->m_Team;
			if(pData->m_Team == 0)
			{
				dbg_assert(!WroteFlagRed, "two red flags");
				DataT.m_CarriedBy = RedFlagCarriedBy;
				WroteFlagRed = true;
			}
			else if(pData->m_Team == 1)
			{
				dbg_assert(!WroteFlagBlue, "two blue flags");
				DataT.m_CarriedBy = BlueFlagCarriedBy;
				WroteFlagBlue = true;
			}
			else
				DataT.m_CarriedBy = -1;
			if(DataT.m_CarriedBy < 0)
				DataT.m_CarriedBy = -1;
			void *pWrite = Builder.NewItem(NewType, ID, sizeof(DataT));
			mem_copy(pWrite, &DataT, sizeof(DataT));
		}
		else if(Type == Protocol6::NETOBJTYPE_GAMEINFO)
		{
			void *pWrite = Builder.NewItem(Protocol5::NETOBJTYPE_GAME, ID, sizeof(GameInfo));
			mem_copy(pWrite, &GameInfo, sizeof(GameInfo));
		}
		else if(Type == Protocol6::NETOBJTYPE_CLIENTINFO)
		{
			Protocol6::CNetObj_ClientInfo *pData = (Protocol6::CNetObj_ClientInfo *)pItem->Data();
			Protocol5::CNetObj_ClientInfo DataT;

			DataT.m_Name0 = pData->m_Name0; // proxy: TODO: utf-8 parsing?
			DataT.m_Name1 = pData->m_Name1;
			DataT.m_Name2 = pData->m_Name2;
			DataT.m_Name3 = pData->m_Name3;
			DataT.m_Name4 = 0;
			DataT.m_Name5 = 0;
			DataT.m_Skin0 = pData->m_Skin0;
			DataT.m_Skin1 = pData->m_Skin1;
			DataT.m_Skin2 = pData->m_Skin2;
			DataT.m_Skin3 = pData->m_Skin3;
			DataT.m_Skin4 = pData->m_Skin4;
			DataT.m_Skin5 = pData->m_Skin5;
			DataT.m_UseCustomColor = pData->m_UseCustomColor;
			DataT.m_ColorBody = pData->m_ColorBody;
			DataT.m_ColorFeet = pData->m_ColorFeet;

			void *pWrite = Builder.NewItem(NewType, ID, sizeof(DataT));
			mem_copy(pWrite, &DataT, sizeof(DataT));
		}
		else
		{
			void *pWrite = Builder.NewItem(NewType, ID, Size);
			mem_copy(pWrite, pItem->Data(), Size);
		}
	}

	return Builder.Finish(pSnap);
}

