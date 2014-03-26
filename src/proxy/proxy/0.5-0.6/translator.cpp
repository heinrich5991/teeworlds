
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

#include "translator.h"

class CTranslator_05_06 : public ITranslator
{
	int m_MapDownloadCrc;
	int m_MapDownloadNum;
public:
	CTranslator_05_06(IHacks *pHacks, PACKET_FUNC pfnTranslatePacketCB, void *pUserData);
	virtual void TranslatePacket(CNetChunk *pPacket);
	virtual int TranslateSnap(CSnapshot *pSnap);
};

class CTranslator_06_05 : public ITranslator
{
	int m_MapDownloadSize;
public:
	CTranslator_06_05(IHacks *pHacks, PACKET_FUNC pfnTranslatePacketCB, void *pUserData);
	virtual void TranslatePacket(CNetChunk *pPacket);
	virtual int TranslateSnap(CSnapshot *pSnap);
};

ITranslator *CreateTranslator_05_06(IHacks *pHacks, PACKET_FUNC pfnTranslatePacketCB, void *pUserData)
{
	return new CTranslator_05_06(pHacks, pfnTranslatePacketCB, pUserData);
}

ITranslator *CreateTranslator_06_05(IHacks *pHacks, PACKET_FUNC pfnTranslatePacketCB, void *pUserData)
{
	return new CTranslator_06_05(pHacks, pfnTranslatePacketCB, pUserData);
}

CTranslator_05_06::CTranslator_05_06(IHacks *pHacks, PACKET_FUNC pfnTranslatePacketCB, void *pUserData)
	: ITranslator(pHacks, pfnTranslatePacketCB, pUserData)
{
	m_MapDownloadCrc = 0;
	m_MapDownloadNum = 0;
}

CTranslator_06_05::CTranslator_06_05(IHacks *pHacks, PACKET_FUNC pfnTranslatePacketCB, void *pUserData)
	: ITranslator(pHacks, pfnTranslatePacketCB, pUserData)
{
	m_MapDownloadSize = 0;
}

void CTranslator_05_06::TranslatePacket(CNetChunk *pPacket)
{
	CUnpacker Unpacker;
	Unpacker.Reset(pPacket->m_pData, pPacket->m_DataSize);
	CPacker Packer;
	Packer.Reset();

	if(pPacket->m_Flags&NETSENDFLAG_CONNLESS)
	{
		// abuse the fact that all messages have the same length
		const unsigned char *pRequest = Unpacker.GetRaw(sizeof(Protocol5::SERVERBROWSE_INFO));
		if(pRequest && mem_comp(pRequest, Protocol5::SERVERBROWSE_INFO,
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
		else if(pPacket->m_DataSize == sizeof(Protocol5::SERVERBROWSE_GETINFO) + 1
			&& mem_comp(pRequest, Protocol5::SERVERBROWSE_GETINFO,
				sizeof(Protocol5::SERVERBROWSE_GETINFO)) == 0)
		{
			Packer.AddRaw(Protocol6::SERVERBROWSE_GETINFO, sizeof(Protocol6::SERVERBROWSE_GETINFO));
			Packer.AddRaw(Unpacker.GetRaw(1), 1); // token
		}
		else if(pPacket->m_DataSize == sizeof(Protocol5::SERVERBROWSE_OLD_GETINFO)
			&& mem_comp(pRequest, Protocol5::SERVERBROWSE_OLD_GETINFO,
				sizeof(Protocol5::SERVERBROWSE_OLD_GETINFO)) == 0)
		{
			static const unsigned char Byte255 = 255; // just hope we never hit 255 refreshs
			Packer.AddRaw(Protocol6::SERVERBROWSE_GETINFO, sizeof(Protocol6::SERVERBROWSE_GETINFO));
			Packer.AddRaw(&Byte255, 1);
		}
		else
			return;

		CNetChunk Packet = *pPacket;
		Packet.m_DataSize = Packer.Size();
		Packet.m_pData = Packer.Data();
		TranslatePacketCB(&Packet);
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

		// client -> server
		if(Msg == Protocol5::NETMSG_INFO)
		{
			const char *pVersion = Unpacker.GetString(CUnpacker::SANITIZE_CC);
			if(str_comp(pVersion, Protocol5::GAME_NETVERSION) == 0)
				Packer.AddString(Protocol6::GAME_NETVERSION, 0);
			else
				Packer.AddString(pVersion, 0);
			Unpacker.GetString(); // player name
			Unpacker.GetString(); // player clan
		}
		else if(Msg == Protocol5::NETMSG_INPUT)
		{
			int Size = 0;
			Packer.AddInt(Unpacker.GetInt()); // acked snapshot
			Packer.AddInt(Unpacker.GetInt()); // intended tick
			Packer.AddInt(Size = Unpacker.GetInt()); // size
			if(Unpacker.Error() || Size / 4 > Protocol6::MAX_INPUT_SIZE || Size < (int)sizeof(Protocol5::CNetObj_PlayerInput))
				return;
			int aInputBuf[Protocol6::MAX_INPUT_SIZE];
			for(int i = 0; i < Size / 4; i++)
				aInputBuf[i] = Unpacker.GetInt();

			if(Unpacker.Error())
				return;

			// abuse that both versions have the same input layout
			Protocol5::CNetObj_PlayerInput *pData = (Protocol5::CNetObj_PlayerInput *)aInputBuf;
			Protocol6::CNetObj_PlayerInput DataT = *(Protocol6::CNetObj_PlayerInput *)pData;

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

			mem_copy(aInputBuf, &DataT, sizeof(DataT));
			for(int i = 0; i < Size / 4; i++)
				Packer.AddInt(aInputBuf[i]);
		}
		// server -> client
		else if(Msg == Protocol5::NETMSG_MAP_CHANGE)
		{
			CNetChunk Packet = *pPacket;

			Packer.AddString(Unpacker.GetString(), 0);           // map name
			Packer.AddInt(m_MapDownloadCrc = Unpacker.GetInt()); // map crc
			Packer.AddInt(1); // proxy: TODO: fix me             // map size
			m_MapDownloadNum = 0;

			Packet.m_DataSize = Packer.Size();
			Packet.m_pData = Packer.Data();
			TranslatePacketCB(&Packet);

			Packer.Reset();
			Packer.AddInt(Protocol6::NETMSG_CON_READY << 1 | 1);

			Packet.m_DataSize = Packer.Size();
			Packet.m_pData = Packer.Data();
			TranslatePacketCB(&Packet);
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
		if(Msg == Protocol5::NETMSGTYPE_CL_STARTINFO
			|| Msg == Protocol5::NETMSGTYPE_CL_CHANGEINFO
			|| Msg == Protocol5::NETMSGTYPE_CL_EMOTICON
			|| Msg == Protocol5::NETMSGTYPE_CL_CALLVOTE)
		{
			pRawData = Handler.SecureUnpackMsg(Msg, &Unpacker);
			if(!pRawData)
				return;
		}

		// client -> server
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
			DataT.Pack((CMsgPacker *)&Packer);
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
		// server -> client
	}

	Packer.AddRaw(&Unpacker);
	CNetChunk Packet = *pPacket;
	Packet.m_DataSize = Packer.Size();
	Packet.m_pData = Packer.Data();
	TranslatePacketCB(&Packet);
}

void CTranslator_06_05::TranslatePacket(CNetChunk *pPacket)
{
	CUnpacker Unpacker;
	Unpacker.Reset(pPacket->m_pData, pPacket->m_DataSize);
	CPacker Packer;
	Packer.Reset();

	if(pPacket->m_Flags&NETSENDFLAG_CONNLESS)
	{
		const unsigned char *pRequest = Unpacker.GetRaw(sizeof(Protocol6::SERVERBROWSE_INFO));
		if(pRequest && mem_comp(pRequest, Protocol6::SERVERBROWSE_INFO,
			sizeof(Protocol6::SERVERBROWSE_INFO)) == 0)
		{
			const char *pString = Unpacker.GetString();
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
				//       x    progression
				//     6   x  player count
				//     7   x  max players
				//     8 x x  client count
				//     9 x x  max clients
				// 5k+10 x x  k-th client's name
				// 5k+11   x  k-th client's clan
				// 5k+12   x  k-th client's country
				// 5k+13 x x  k-th client's score
				// 5k+14   x  k-th client's team

				if(i == 0) // token
				{
					if(str_toint(pString) != 255) // magic number for SERVERBROWSE_OLD_INFO
					{
						Packer.AddRaw(Protocol5::SERVERBROWSE_INFO, sizeof(Protocol5::SERVERBROWSE_INFO));
						Packer.AddString(pString, 0);
						//dbg_msg("packer", "adding '%s'", pString);
					}
					else
					{
						Packer.AddRaw(Protocol5::SERVERBROWSE_OLD_INFO, sizeof(Protocol5::SERVERBROWSE_OLD_INFO));
						//dbg_msg("packer", "dropping '%s'", pString);
					}
				}
				else if(i == 1) // version
				{
					Packer.AddString("0.5.2", 0);
					//dbg_msg("packer", "adding '0.5.2'");
				}
				else if(i == 6 || i == 7
					|| (i >= 10 && (i % 5 == 1 || i % 5 == 2 || i % 5 == 4)))
				{
					// nothing
					//dbg_msg("packer", "dropping '%s'", pString);
				}
				else
				{
					Packer.AddString(pString, 0);
					//dbg_msg("packer", "adding '%s'", pString);
				}

				if(i == 5) // progression
				{
					Packer.AddString("0", 0);
					//dbg_msg("packer", "inserting '0'");
				}

				if(Unpacker.Remaining() <= 0)
					break;
				pString = Unpacker.GetString();
				i++;
			}
		}
		else if(pPacket->m_DataSize == sizeof(Protocol6::SERVERBROWSE_GETINFO) + 1
			&& mem_comp(pRequest, Protocol6::SERVERBROWSE_GETINFO,
				sizeof(Protocol6::SERVERBROWSE_GETINFO)) == 0)
		{
			Packer.AddRaw(Protocol5::SERVERBROWSE_GETINFO, sizeof(Protocol5::SERVERBROWSE_GETINFO));
			Packer.AddRaw(Unpacker.GetRaw(1), 1); // token
		}

		CNetChunk Packet = *pPacket;
		Packet.m_DataSize = Packer.Size();
		Packet.m_pData = Packer.Data();
		TranslatePacketCB(&Packet);
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
		// server -> client
		if(Msg == Protocol6::NETMSG_MAP_CHANGE)
		{
			Packer.AddString(Unpacker.GetString(), 0); // map name
			Packer.AddInt(Unpacker.GetInt());          // map crc
			m_MapDownloadSize = Unpacker.GetInt();     // map size
		}
		else if(Msg == Protocol6::NETMSG_MAP_DATA)
		{
			Packer.AddInt(Unpacker.GetInt()); // map chunk is the last one?
			Packer.AddInt(m_MapDownloadSize); // map size
			Unpacker.GetInt();                // map crc
			Unpacker.GetInt();                // map chunk num
			Packer.AddInt(Unpacker.GetInt()); // map chunk size
			                                  // map chunk
		}
		// client -> server
		else if(Msg == Protocol6::NETMSG_INFO)
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
		if(Msg == Protocol6::NETMSGTYPE_SV_EMOTICON
			|| Msg == Protocol6::NETMSGTYPE_SV_VOTEOPTIONLISTADD
			|| Msg == Protocol6::NETMSGTYPE_SV_VOTEOPTIONREMOVE
			|| Msg == Protocol6::NETMSGTYPE_CL_STARTINFO
			|| Msg == Protocol6::NETMSGTYPE_CL_CHANGEINFO
			|| Msg == Protocol6::NETMSGTYPE_CL_CALLVOTE)
		{
			Protocol6::CNetObjHandler Handler;
			pRawData = Handler.SecureUnpackMsg(Msg, &Unpacker);
			if(!pRawData)
				return;
		}

		// server -> client
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
				Packer.Reset();
				Packer.AddInt(Protocol5::NETMSGTYPE_SV_VOTEOPTION << 1 | Sys);

				DataT.m_pCommand = m_apVoteOptions[i];

				DataT.Pack((CMsgPacker *)&Packer);
				Packet.m_DataSize = Packer.Size();
				Packet.m_pData = Packer.Data();
				TranslatePacketCB(&Packet);
			}

			return;
		}
		// client -> server
		else if(Msg == Protocol6::NETMSGTYPE_CL_STARTINFO
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
	TranslatePacketCB(&Packet);
}

int CTranslator_05_06::TranslateSnap(CSnapshot *pSnap)
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
			Protocol6::CNetObj_Flag *pData = (Protocol6::CNetObj_Flag *)pItem->Data();
			Protocol5::CNetObj_Flag DataT;

			DataT.m_X = pData->m_X;
			DataT.m_Y = pData->m_Y;
			DataT.m_Team = pData->m_Team;
			if(pData->m_Team == 0)
			{
				if(!FoundRedFlag)
					RedFlagCarriedBy = DataT.m_CarriedBy;
				else
					RedFlagCarriedBy = -1;
				FoundRedFlag = true;
			}
			else if(pData->m_Team == 1)
			{
				if(!FoundBlueFlag)
					BlueFlagCarriedBy = DataT.m_CarriedBy;
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
		else if(Type == Protocol6::NETOBJTYPE_CLIENTINFO)
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
			DataT.m_Clan0 = 0;
			DataT.m_Clan1 = 0;
			DataT.m_Clan2 = 0;
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

int CTranslator_06_05::TranslateSnap(CSnapshot *pSnap)
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

