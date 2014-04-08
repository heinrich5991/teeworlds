
#include "proxy.h"

#include <proxy/proxy/proxy.h>
#include <proxy/proxy/proxy_macros.h>

#include <engine/server.h>

#include <engine/shared/network.h>
#include <engine/shared/packer.h>
#include <engine/shared/snapshot.h>

#include <proxy/proxy/0.5/nethash.h>
#include <proxy/proxy/0.5/mastersrv.h>
#include <proxy/proxy/0.5/protocol.h>
#include <proxy/proxy/0.5/protocol_generated.h>

#include <proxy/proxy/0.6/nethash.h>
#include <proxy/proxy/0.6/mastersrv.h>
#include <proxy/proxy/0.6/protocol.h>
#include <proxy/proxy/0.6/protocol_generated.h>

class CProxy_Client06_Server05 : public IProxy
{
	int m_MapDownloadCrc;
	int m_MapDownloadNum;
public:
	CProxy_Client06_Server05();
	virtual void TranslateClientPacket(CNetChunk *pPacket, CProxyCB ClientCB, CProxyCB ServerCB);
	virtual void TranslateServerPacket(CNetChunk *pPacket, CProxyCB ClientCB, CProxyCB ServerCB);
	virtual int TranslateServerSnap(CSnapshot *pSnap);
};

IProxy *CreateProxy_Client06_Server05()
{
	return new CProxy_Client06_Server05();
}

CProxy_Client06_Server05::CProxy_Client06_Server05()
	: IProxy()
{
	m_MapDownloadCrc = 0;
	m_MapDownloadNum = 0;
}

void CProxy_Client06_Server05::TranslateClientPacket(CNetChunk *pPacket, CProxyCB ClientCB, CProxyCB ServerCB)
{
	CUnpacker Unpacker;
	Unpacker.Reset(pPacket->m_pData, pPacket->m_DataSize);
	CPacker Packer;
	Packer.Reset();

	if(pPacket->m_Flags&NETSENDFLAG_CONNLESS)
	{
		const unsigned char *pRequest = Unpacker.GetRaw(sizeof(Protocol6::SERVERBROWSE_INFO));
		if(Unpacker.Error())
			return;

		if(mem_comp(pRequest, Protocol6::SERVERBROWSE_GETINFO,
			sizeof(Protocol6::SERVERBROWSE_GETINFO)) == 0)
		{
			Packer.AddRaw(Protocol5::SERVERBROWSE_GETINFO, sizeof(Protocol5::SERVERBROWSE_GETINFO));
			Packer.AddRaw(Unpacker.GetRaw(1), 1); // token
		}
		else if(mem_comp(pRequest, Protocol6::SERVERBROWSE_GETLIST,
			sizeof(Protocol6::SERVERBROWSE_GETLIST)) == 0)
		{
			Packer.AddRaw(Protocol5::SERVERBROWSE_GETLIST, sizeof(Protocol5::SERVERBROWSE_GETLIST));
		}
		else
			return;

		CNetChunk Packet = *pPacket;
		Packet.m_DataSize = Packer.Size();
		Packet.m_pData = Packer.Data();
		PROXY_CLIENT_PACKET(&Packet);
		return;
	}

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
			// the new msg 'NETMSG_CON_READY' was added
			MsgT = Msg - 1;

		Packer.AddInt(MsgT << 1 | Sys);

		if(Msg == Protocol6::NETMSG_INFO)
		{
			const char *pVersion = Unpacker.GetString(CUnpacker::SANITIZE_CC);
			if(str_comp(pVersion, Protocol6::GAME_NETVERSION) == 0)
				Packer.AddString(Protocol5::GAME_NETVERSION, 0);
			else
				Packer.AddString(pVersion, 0);
			Packer.AddString("", 0); // player name
			Packer.AddString("", 0); // player clan
		}
		else if(Msg == Protocol6::NETMSG_INPUT)
		{
			int Size = 0;
			Packer.AddInt(Unpacker.GetInt()); // acked snapshot
			Packer.AddInt(Unpacker.GetInt()); // intended tick
			Packer.AddInt(Size = Unpacker.GetInt()); // size
			if(Unpacker.Error() || Size / 4 > Protocol5::MAX_INPUT_SIZE || Size < (int)sizeof(Protocol6::CNetObj_PlayerInput))
				return;
			int aInputBuf[Protocol5::MAX_INPUT_SIZE];
			for(int i = 0; i < Size / 4; i++)
				aInputBuf[i] = Unpacker.GetInt();

			if(Unpacker.Error())
				return;

			// abuse that both versions have the same input layout
			Protocol6::CNetObj_PlayerInput *pData = (Protocol6::CNetObj_PlayerInput *)aInputBuf;
			Protocol5::CNetObj_PlayerInput DataT = *(Protocol5::CNetObj_PlayerInput *)pData;

			DataT.m_PlayerState = Protocol5::PLAYERSTATE_UNKNOWN;
			if(pData->m_PlayerFlags&Protocol6::PLAYERFLAG_IN_MENU)
				DataT.m_PlayerState = Protocol5::PLAYERSTATE_IN_MENU;
			else if(pData->m_PlayerFlags&Protocol6::PLAYERFLAG_CHATTING)
				DataT.m_PlayerState = Protocol5::PLAYERSTATE_CHATTING;
			else if(pData->m_PlayerFlags&Protocol6::PLAYERFLAG_PLAYING)
				DataT.m_PlayerState = Protocol5::PLAYERSTATE_PLAYING;


			mem_copy(aInputBuf, &DataT, sizeof(DataT));
			for(int i = 0; i < Size / 4; i++)
				Packer.AddInt(aInputBuf[i]);
		}
	}
	else
	{
		int MsgT = Msg;

		if(Msg == Protocol6::NETMSGTYPE_SV_VOTEOPTIONREMOVE // proxy: TODO: add support here
			|| Msg == Protocol6::NETMSGTYPE_CL_SETSPECTATORMODE)
			return;

		if(Msg == Protocol6::NETMSGTYPE_SV_VOTEOPTIONADD)
			MsgT = Protocol5::NETMSGTYPE_SV_VOTEOPTION;
		if(Msg >= Protocol6::NETMSGTYPE_SV_VOTESET)
			MsgT = Msg - 2;
		if(Msg >= Protocol6::NETMSGTYPE_CL_STARTINFO)
			MsgT = Msg - 3;

		Packer.AddInt(MsgT << 1 | Sys);

		void *pRawData;
		if(Msg == Protocol6::NETMSGTYPE_CL_STARTINFO
			|| Msg == Protocol6::NETMSGTYPE_CL_CHANGEINFO
			|| Msg == Protocol6::NETMSGTYPE_CL_CALLVOTE)
		{
			Protocol6::CNetObjHandler Handler;
			pRawData = Handler.SecureUnpackMsg(Msg, &Unpacker);
			if(!pRawData)
				return;
		}

		if(Msg == Protocol6::NETMSGTYPE_CL_STARTINFO
			|| Msg == Protocol6::NETMSGTYPE_CL_CHANGEINFO)
		{
			Protocol6::CNetMsg_Cl_StartInfo *pData = (Protocol6::CNetMsg_Cl_StartInfo *)pRawData;
			Protocol5::CNetMsg_Cl_StartInfo DataT;
			DataT.m_pName = pData->m_pName;
			//pData->m_pClan;
			//pData->m_Country;
			DataT.m_pSkin = pData->m_pSkin;
			DataT.m_UseCustomColor = pData->m_UseCustomColor;
			DataT.m_ColorBody = pData->m_ColorBody;
			DataT.m_ColorFeet = pData->m_ColorFeet;
			DataT.Pack((CMsgPacker *)&Packer);
		}
		else if(Msg == Protocol6::NETMSGTYPE_CL_CALLVOTE)
		{
			Protocol6::CNetMsg_Cl_CallVote *pData = (Protocol6::CNetMsg_Cl_CallVote *)pRawData;
			Protocol5::CNetMsg_Cl_CallVote DataT;
			DataT.m_Type = pData->m_Type;
			DataT.m_Value = pData->m_Value;
			//Data->m_Reason;
			DataT.Pack((CMsgPacker *)&Packer);
		}
	}

	Packer.AddRaw(&Unpacker);
	CNetChunk Packet = *pPacket;
	Packet.m_DataSize = Packer.Size();
	Packet.m_pData = Packer.Data();
	PROXY_CLIENT_PACKET(&Packet);
}

void CProxy_Client06_Server05::TranslateServerPacket(CNetChunk *pPacket, CProxyCB ClientCB, CProxyCB ServerCB)
{
	CUnpacker Unpacker;
	Unpacker.Reset(pPacket->m_pData, pPacket->m_DataSize);
	CPacker Packer;
	Packer.Reset();

	if(pPacket->m_Flags&NETSENDFLAG_CONNLESS)
	{
		// abuse the fact that all messages have the same length
		const unsigned char *pRequest = Unpacker.GetRaw(sizeof(Protocol5::SERVERBROWSE_INFO));
		if(Unpacker.Error())
			return;

		if(mem_comp(pRequest, Protocol5::SERVERBROWSE_INFO,
			sizeof(Protocol5::SERVERBROWSE_INFO)) == 0)
		{
			Packer.AddRaw(Protocol6::SERVERBROWSE_INFO, sizeof(Protocol6::SERVERBROWSE_INFO));

			const char *pString = Unpacker.GetString();
			const char *pClientCount = 0;
			int i = 0;
			while(!Unpacker.Error())
			{
				//     i 5 6  desc
				//     0 x x  token
				//     1 m x  version
				//     2 x x  name
				//     3 x x  map
				//     4 x x  gametype
				//     5 x x  flags
				//     6 x    progression
				//         x  player count
				//         x  max players
				//     7 x x  client count
				//     8 x x  max clients
				// 2k+ 9 x x  k-th client's name
				//         x  k-th client's clan
				//         x  k-th client's country
				// 2k+10 x x  k-th client's score
				//         x  k-th client's team

				if(i == 1) // version
				{
					char aBuf[256];
					str_format(aBuf, sizeof(aBuf), "0.6 %s", pString);
					Packer.AddString(aBuf, 0);
					//dbg_msg("packer", "adding '%s'", aBuf);
				}
				else if(i == 2) // server name
				{
					char aBuf[256];
					str_format(aBuf, sizeof(aBuf), "[0.5] %s", pString);
					Packer.AddString(aBuf, 0);
					//dbg_msg("packer", "adding '%s'", aBuf);
				}
				else if(i == 6) // progression
				{
					// nothing
					//dbg_msg("packer", "dropping '%s'", pString);
				}
				else
				{
					Packer.AddString(pString, 0);
					//dbg_msg("packer", "adding '%s'", pString);
				}

				if(i == 7)
				{
					pClientCount = pString;
				}
				if(i == 8)
				{
					//dbg_msg("packer", "inserting '%s'", pClientCount);
					Packer.AddString(pClientCount, 0); // client_count
					//dbg_msg("packer", "inserting '%s'", pString);
					Packer.AddString(pString, 0); // max_clients
				}
				if(i >= 9 && i % 2 == 1) // add client's clan and country
				{
					//dbg_msg("packer", "inserting '%s'", "");
					Packer.AddString("", 0); // clan
					//dbg_msg("packer", "inserting '%s'", "-1");
					Packer.AddString("-1", 0); // country
				}
				if(i >= 9 && i % 2 == 0) // add client's team
				{
					//dbg_msg("packer", "inserting '%s'", "0");
					Packer.AddString("0", 0); // team
				}

				if(Unpacker.Remaining() <= 0)
					break;
				pString = Unpacker.GetString();
				i++;
			}
		}
		else if(mem_comp(pRequest, Protocol5::SERVERBROWSE_LIST,
			sizeof(Protocol5::SERVERBROWSE_LIST)) == 0)
		{
			static unsigned char IPV4Mapping[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF };
			static const int MAX_SERVERS_PER_PACKET_6 = 75;

			CNetChunk Packet = *pPacket;

			int NumServers = 0;

			Protocol5::MASTERSRV_ADDR *pAddr = (Protocol5::MASTERSRV_ADDR *)Unpacker.GetRaw(sizeof(Protocol5::MASTERSRV_ADDR));

			for(int i = 0; !Unpacker.Error(); i++)
			{
				if(NumServers == 0)
					Packer.AddRaw(Protocol6::SERVERBROWSE_LIST, sizeof(Protocol6::SERVERBROWSE_LIST));

				Protocol6::CMastersrvAddr AddrT;
				mem_zero(&AddrT, sizeof(AddrT));

				mem_copy(&AddrT, IPV4Mapping, sizeof(IPV4Mapping));
				for(unsigned k = 0; k < sizeof(pAddr->m_aIp); k++)
					AddrT.m_aIp[sizeof(IPV4Mapping) + k] = pAddr->m_aIp[k];

				// swapping is intentional, that changed from 0.5 to 0.6
				AddrT.m_aPort[0] = pAddr->m_aPort[1];
				AddrT.m_aPort[1] = pAddr->m_aPort[0];

				Packer.AddRaw(&AddrT, sizeof(AddrT));

				NumServers++;

				pAddr = (Protocol5::MASTERSRV_ADDR *)Unpacker.GetRaw(sizeof(Protocol5::MASTERSRV_ADDR));

				if(NumServers == MAX_SERVERS_PER_PACKET_6 || Unpacker.Error())
				{
					Packet.m_DataSize = Packer.Size();
					Packet.m_pData = Packer.Data();
					PROXY_SERVER_PACKET(&Packet);

					Packer.Reset();
					NumServers = 0;
				}
			}
		}
		else
			return;

		CNetChunk Packet = *pPacket;
		Packet.m_DataSize = Packer.Size();
		Packet.m_pData = Packer.Data();
		PROXY_SERVER_PACKET(&Packet);
		return;
	}

	int Msg = Unpacker.GetInt();
	int Sys = Msg&1;
	Msg >>= 1;

	if(Unpacker.Error())
		return;

	if(Sys)
	{
		int MsgT = Msg;
		if(Msg >= Protocol5::NETMSG_SNAP)
			// a new msg 'NETMSG_CON_READY' was added between the other messages
			MsgT = Msg + 1;

		Packer.AddInt(MsgT << 1 | Sys);

		if(Msg == Protocol5::NETMSG_MAP_CHANGE)
		{
			CNetChunk Packet = *pPacket;

			Packer.AddString(Unpacker.GetString(), 0);           // map name
			Packer.AddInt(m_MapDownloadCrc = Unpacker.GetInt()); // map crc
			Packer.AddInt(1); // proxy: TODO: fix me             // map size
			m_MapDownloadNum = 0;

			Packet.m_DataSize = Packer.Size();
			Packet.m_pData = Packer.Data();
			PROXY_SERVER_PACKET(&Packet);

			Packer.Reset();
			Packer.AddInt(Protocol6::NETMSG_CON_READY << 1 | 1);

			Packet.m_DataSize = Packer.Size();
			Packet.m_pData = Packer.Data();
			PROXY_SERVER_PACKET(&Packet);
			return;
		}
		else if(Msg == Protocol5::NETMSG_MAP_DATA)
		{
			Packer.AddInt(Unpacker.GetInt());  // map chunk is the last one?
			Unpacker.GetInt();                 // map size
			Packer.AddInt(m_MapDownloadCrc);   // map crc
			Packer.AddInt(m_MapDownloadNum++); // map chunk num
			Packer.AddInt(Unpacker.GetInt());  // map chunk size
			                                   // map chunk
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
		if(false)// Msg == Protocol5::NETMSGTYPE_CL_STARTINFO)
		{
			pRawData = Handler.SecureUnpackMsg(Msg, &Unpacker);
			if(!pRawData)
				return;
		}
	}

	Packer.AddRaw(&Unpacker);
	CNetChunk Packet = *pPacket;
	Packet.m_DataSize = Packer.Size();
	Packet.m_pData = Packer.Data();
	PROXY_SERVER_PACKET(&Packet);
}

int CProxy_Client06_Server05::TranslateServerSnap(CSnapshot *pSnap)
{
	CSnapshotBuilder Builder;
	Builder.Init();

	bool FoundRedFlag = false, FoundBlueFlag = false;
	int RedFlagCarriedBy = -1, BlueFlagCarriedBy = -1;

	// first, gather some info
	for(int i = 0; i < pSnap->NumItems(); i++)
	{
		CSnapshotItem *pItem = pSnap->GetItem(i);
		if(pItem->Type() == Protocol5::NETOBJTYPE_FLAG)
		{
			Protocol5::CNetObj_Flag *pData = (Protocol5::CNetObj_Flag *)pItem->Data();

			if(pData->m_Team == 0)
			{
				if(!FoundRedFlag)
					RedFlagCarriedBy = pData->m_CarriedBy;
				else
					RedFlagCarriedBy = -1;
				FoundRedFlag = true;
			}
			else if(pData->m_Team == 1)
			{
				if(!FoundBlueFlag)
					BlueFlagCarriedBy = pData->m_CarriedBy;
				else
					RedFlagCarriedBy = -1;
				FoundBlueFlag = true;
			}
		}
	}

	// second, build new snapshot
	for(int i = 0; i < pSnap->NumItems(); i++)
	{
		CSnapshotItem *pItem = pSnap->GetItem(i);
		int Type = pItem->Type();
		int ID = pItem->ID();
		int Size = pSnap->GetItemSize(i);

		int NewType = Type;
		if(Type >= Protocol5::NETOBJTYPE_CHARACTERCORE)
			NewType = Type + 1;
		if(Type >= Protocol5::NETEVENTTYPE_COMMON)
			NewType = Type + 2;

		if(Type == Protocol5::NETOBJTYPE_CHARACTER)
		{
			Protocol5::CNetObj_Character *pData = (Protocol5::CNetObj_Character *)pItem->Data();

			// abuse that it has the same layout
			Protocol6::CNetObj_Character DataT = *(Protocol6::CNetObj_Character *)pData;

			DataT.m_PlayerFlags = 0;
			DataT.m_PlayerFlags |= Protocol6::PLAYERFLAG_SCOREBOARD; // to fix ping updates
			if(pData->m_PlayerState == Protocol5::PLAYERSTATE_IN_MENU)
			{
				DataT.m_PlayerFlags |= Protocol6::PLAYERFLAG_IN_MENU;
				DataT.m_PlayerFlags &= ~Protocol6::PLAYERFLAG_SCOREBOARD; // no possibility to have scoreboard open
			}
			else if(pData->m_PlayerState == Protocol5::PLAYERSTATE_CHATTING)
				DataT.m_PlayerFlags |= Protocol6::PLAYERFLAG_CHATTING;
			else
				DataT.m_PlayerFlags |= Protocol6::PLAYERFLAG_PLAYING;
			void *pWrite = Builder.NewItem(NewType, ID, sizeof(DataT));
			mem_copy(pWrite, &DataT, sizeof(DataT));
		}
		else if(Type == Protocol5::NETOBJTYPE_PLAYERINFO)
		{
			Protocol5::CNetObj_PlayerInfo *pData = (Protocol5::CNetObj_PlayerInfo *)pItem->Data();
			Protocol6::CNetObj_PlayerInfo DataT;
			// 0.6 only got the last field removed, simply mem_copy it
			mem_zero(&DataT, sizeof(DataT));
			mem_copy(&DataT, pData, sizeof(DataT));

			void *pWrite = Builder.NewItem(NewType, ID, sizeof(DataT));
			mem_copy(pWrite, &DataT, sizeof(DataT));
		}
		else if(Type == Protocol5::NETOBJTYPE_FLAG)
		{
			Protocol5::CNetObj_Flag *pData = (Protocol5::CNetObj_Flag *)pItem->Data();
			Protocol6::CNetObj_Flag DataT;

			DataT.m_X = pData->m_X;
			DataT.m_Y = pData->m_Y;
			DataT.m_Team = pData->m_Team;
			//pData->m_CarriedBy;
			void *pWrite = Builder.NewItem(NewType, ID, sizeof(DataT));
			mem_copy(pWrite, &DataT, sizeof(DataT));
		}
		else if(Type == Protocol5::NETOBJTYPE_GAME)
		{
			Protocol5::CNetObj_Game *pData = (Protocol5::CNetObj_Game *)pItem->Data();
			Protocol6::CNetObj_GameInfo DataT;

			DataT.m_RoundStartTick = pData->m_RoundStartTick;
			DataT.m_GameFlags = pData->m_Flags;
			DataT.m_GameStateFlags = 0;
			if(pData->m_GameOver)
				DataT.m_GameStateFlags |= Protocol6::GAMESTATEFLAG_GAMEOVER;
			if(pData->m_SuddenDeath)
				DataT.m_GameStateFlags |= Protocol6::GAMESTATEFLAG_SUDDENDEATH;
			if(pData->m_Paused)
				DataT.m_GameStateFlags |= Protocol6::GAMESTATEFLAG_PAUSED;
			DataT.m_ScoreLimit = pData->m_ScoreLimit;
			DataT.m_TimeLimit = pData->m_TimeLimit;
			DataT.m_WarmupTimer = pData->m_Warmup;
			DataT.m_RoundNum = pData->m_RoundNum;
			DataT.m_RoundCurrent = pData->m_RoundCurrent;

			if(pData->m_Flags)
			{
				Protocol6::CNetObj_GameData DataT;
				DataT.m_TeamscoreRed = pData->m_TeamscoreRed;
				DataT.m_TeamscoreBlue = pData->m_TeamscoreBlue;
				DataT.m_FlagCarrierRed = RedFlagCarriedBy;
				DataT.m_FlagCarrierBlue = BlueFlagCarriedBy;

				void *pWrite = Builder.NewItem(Protocol6::NETOBJTYPE_GAMEDATA, ID, sizeof(DataT));
				mem_copy(pWrite, &DataT, sizeof(DataT));
			}

			void *pWrite = Builder.NewItem(Protocol6::NETOBJTYPE_GAMEINFO, ID, sizeof(DataT));
			mem_copy(pWrite, &DataT, sizeof(DataT));
		}
		else if(Type == Protocol5::NETOBJTYPE_CLIENTINFO)
		{
			Protocol5::CNetObj_ClientInfo *pData = (Protocol5::CNetObj_ClientInfo *)pItem->Data();
			Protocol6::CNetObj_ClientInfo DataT;

			DataT.m_Name0 = pData->m_Name0; // proxy: TODO: utf-8 parsing?
			DataT.m_Name1 = pData->m_Name1;
			DataT.m_Name2 = pData->m_Name2;
			DataT.m_Name3 = pData->m_Name3;
			//pData->m_Name4;
			//pData->m_Name5;
			DataT.m_Skin0 = pData->m_Skin0;
			DataT.m_Skin1 = pData->m_Skin1;
			DataT.m_Skin2 = pData->m_Skin2;
			DataT.m_Skin3 = pData->m_Skin3;
			DataT.m_Skin4 = pData->m_Skin4;
			DataT.m_Skin5 = pData->m_Skin5;
			DataT.m_Country = -1;
			DataT.m_Clan0 = 0x80808080; // empty string
			DataT.m_Clan1 = 0x80808080;
			DataT.m_Clan2 = 0x80808080;
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
