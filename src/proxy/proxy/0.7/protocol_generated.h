#ifndef PROXY_07_PROTOCOL_H
#define PROXY_07_PROTOCOL_H


#include <engine/message.h>

namespace Protocol7
{
enum
{
	INPUT_STATE_MASK=0x3f
};

enum
{
	TEAM_SPECTATORS=-1,
	TEAM_RED,
	TEAM_BLUE,
	NUM_TEAMS,

	FLAG_MISSING=-3,
	FLAG_ATSTAND,
	FLAG_TAKEN,

	SPEC_FREEVIEW=-1,
};

enum
{
	PICKUP_HEALTH=0,
	PICKUP_ARMOR,
	PICKUP_GRENADE,
	PICKUP_SHOTGUN,
	PICKUP_LASER,
	PICKUP_NINJA,
	NUM_PICKUPS
};

enum
{
	EMOTE_NORMAL=0,
	EMOTE_PAIN,
	EMOTE_HAPPY,
	EMOTE_SURPRISE,
	EMOTE_ANGRY,
	EMOTE_BLINK,
	NUM_EMOTES
};

enum
{
	EMOTICON_OOP=0,
	EMOTICON_EXCLAMATION,
	EMOTICON_HEARTS,
	EMOTICON_DROP,
	EMOTICON_DOTDOT,
	EMOTICON_MUSIC,
	EMOTICON_SORRY,
	EMOTICON_GHOST,
	EMOTICON_SUSHI,
	EMOTICON_SPLATTEE,
	EMOTICON_DEVILTEE,
	EMOTICON_ZOMG,
	EMOTICON_ZZZ,
	EMOTICON_WTF,
	EMOTICON_EYES,
	EMOTICON_QUESTION,
	NUM_EMOTICONS
};

enum
{
	VOTE_UNKNOWN=0,
	VOTE_START_OP,
	VOTE_START_KICK,
	VOTE_START_SPEC,
	VOTE_END_ABORT,
	VOTE_END_PASS,
	VOTE_END_FAIL,
	NUM_VOTES
};

enum
{
	GAMEMSG_TEAM_SWAP=0,
	GAMEMSG_SPEC_INVALIDID,
	GAMEMSG_TEAM_SHUFFLE,
	GAMEMSG_TEAM_BALANCE,
	GAMEMSG_CTF_DROP,
	GAMEMSG_CTF_RETURN,
	GAMEMSG_TEAM_ALL,
	GAMEMSG_TEAM_BALANCE_VICTIM,
	GAMEMSG_CTF_GRAB,
	GAMEMSG_CTF_CAPTURE,
	NUM_GAMEMSGS
};

enum
{
	PLAYERFLAG_ADMIN = 1<<0,
	PLAYERFLAG_CHATTING = 1<<1,
	PLAYERFLAG_SCOREBOARD = 1<<2,
	PLAYERFLAG_READY = 1<<3,
	PLAYERFLAG_DEAD = 1<<4,
	PLAYERFLAG_WATCHING = 1<<5,
};

enum
{
	GAMEFLAG_TEAMS = 1<<0,
	GAMEFLAG_FLAGS = 1<<1,
	GAMEFLAG_SURVIVAL = 1<<2,
};

enum
{
	GAMESTATEFLAG_WARMUP = 1<<0,
	GAMESTATEFLAG_SUDDENDEATH = 1<<1,
	GAMESTATEFLAG_ROUNDOVER = 1<<2,
	GAMESTATEFLAG_GAMEOVER = 1<<3,
	GAMESTATEFLAG_PAUSED = 1<<4,
	GAMESTATEFLAG_STARTCOUNTDOWN = 1<<5,
};

enum
{
	COREEVENTFLAG_GROUND_JUMP = 1<<0,
	COREEVENTFLAG_AIR_JUMP = 1<<1,
	COREEVENTFLAG_HOOK_ATTACH_PLAYER = 1<<2,
	COREEVENTFLAG_HOOK_ATTACH_GROUND = 1<<3,
	COREEVENTFLAG_HOOK_HIT_NOHOOK = 1<<4,
};

enum
{
	NETOBJ_INVALID=0,
	NETOBJTYPE_PLAYERINPUT,
	NETOBJTYPE_PROJECTILE,
	NETOBJTYPE_LASER,
	NETOBJTYPE_PICKUP,
	NETOBJTYPE_FLAG,
	NETOBJTYPE_GAMEDATA,
	NETOBJTYPE_GAMEDATATEAM,
	NETOBJTYPE_GAMEDATAFLAG,
	NETOBJTYPE_CHARACTERCORE,
	NETOBJTYPE_CHARACTER,
	NETOBJTYPE_PLAYERINFO,
	NETOBJTYPE_SPECTATORINFO,
	NETOBJTYPE_DE_CLIENTINFO,
	NETOBJTYPE_DE_GAMEINFO,
	NETOBJTYPE_DE_TUNEPARAMS,
	NETEVENTTYPE_COMMON,
	NETEVENTTYPE_EXPLOSION,
	NETEVENTTYPE_SPAWN,
	NETEVENTTYPE_HAMMERHIT,
	NETEVENTTYPE_DEATH,
	NETEVENTTYPE_SOUNDWORLD,
	NETEVENTTYPE_DAMAGEIND,
	NUM_NETOBJTYPES
};

enum
{
	NETMSG_INVALID=0,
	NETMSGTYPE_SV_MOTD,
	NETMSGTYPE_SV_CHAT,
	NETMSGTYPE_SV_TEAM,
	NETMSGTYPE_SV_KILLMSG,
	NETMSGTYPE_SV_TUNEPARAMS,
	NETMSGTYPE_SV_EXTRAPROJECTILE,
	NETMSGTYPE_SV_READYTOENTER,
	NETMSGTYPE_SV_WEAPONPICKUP,
	NETMSGTYPE_SV_EMOTICON,
	NETMSGTYPE_SV_VOTECLEAROPTIONS,
	NETMSGTYPE_SV_VOTEOPTIONLISTADD,
	NETMSGTYPE_SV_VOTEOPTIONADD,
	NETMSGTYPE_SV_VOTEOPTIONREMOVE,
	NETMSGTYPE_SV_VOTESET,
	NETMSGTYPE_SV_VOTESTATUS,
	NETMSGTYPE_SV_SERVERSETTINGS,
	NETMSGTYPE_SV_CLIENTINFO,
	NETMSGTYPE_SV_GAMEINFO,
	NETMSGTYPE_SV_CLIENTDROP,
	NETMSGTYPE_SV_GAMEMSG,
	NETMSGTYPE_DE_CLIENTENTER,
	NETMSGTYPE_DE_CLIENTLEAVE,
	NETMSGTYPE_CL_SAY,
	NETMSGTYPE_CL_SETTEAM,
	NETMSGTYPE_CL_SETSPECTATORMODE,
	NETMSGTYPE_CL_STARTINFO,
	NETMSGTYPE_CL_KILL,
	NETMSGTYPE_CL_READYCHANGE,
	NETMSGTYPE_CL_EMOTICON,
	NETMSGTYPE_CL_VOTE,
	NETMSGTYPE_CL_CALLVOTE,
	NUM_NETMSGTYPES
};

struct CNetObj_PlayerInput
{
	int m_Direction;
	int m_TargetX;
	int m_TargetY;
	int m_Jump;
	int m_Fire;
	int m_Hook;
	int m_PlayerFlags;
	int m_WantedWeapon;
	int m_NextWeapon;
	int m_PrevWeapon;
};

struct CNetObj_Projectile
{
	int m_X;
	int m_Y;
	int m_VelX;
	int m_VelY;
	int m_Type;
	int m_StartTick;
};

struct CNetObj_Laser
{
	int m_X;
	int m_Y;
	int m_FromX;
	int m_FromY;
	int m_StartTick;
};

struct CNetObj_Pickup
{
	int m_X;
	int m_Y;
	int m_Type;
};

struct CNetObj_Flag
{
	int m_X;
	int m_Y;
	int m_Team;
};

struct CNetObj_GameData
{
	int m_GameStartTick;
	int m_GameStateFlags;
	int m_GameStateEndTick;
};

struct CNetObj_GameDataTeam
{
	int m_TeamscoreRed;
	int m_TeamscoreBlue;
};

struct CNetObj_GameDataFlag
{
	int m_FlagCarrierRed;
	int m_FlagCarrierBlue;
	int m_FlagDropTickRed;
	int m_FlagDropTickBlue;
};

struct CNetObj_CharacterCore
{
	int m_Tick;
	int m_X;
	int m_Y;
	int m_VelX;
	int m_VelY;
	int m_Angle;
	int m_Direction;
	int m_Jumped;
	int m_HookedPlayer;
	int m_HookState;
	int m_HookTick;
	int m_HookX;
	int m_HookY;
};

struct CNetObj_Character : public CNetObj_CharacterCore
{
	int m_Health;
	int m_Armor;
	int m_AmmoCount;
	int m_Weapon;
	int m_Emote;
	int m_AttackTick;
	int m_TriggeredEvents;
};

struct CNetObj_PlayerInfo
{
	int m_PlayerFlags;
	int m_Score;
	int m_Latency;
};

struct CNetObj_SpectatorInfo
{
	int m_SpectatorID;
	int m_X;
	int m_Y;
};

struct CNetObj_De_ClientInfo
{
	int m_Local;
	int m_Team;
	int m_aName[4];
	int m_aClan[3];
	int m_Country;
	int m_aaSkinPartNames[6][6];
	int m_aUseCustomColors[6];
	int m_aSkinPartColors[6];
};

struct CNetObj_De_GameInfo
{
	int m_GameFlags;
	int m_ScoreLimit;
	int m_TimeLimit;
	int m_MatchNum;
	int m_MatchCurrent;
};

struct CNetObj_De_TuneParams
{
	int m_aTuneParams[33];
};

struct CNetEvent_Common
{
	int m_X;
	int m_Y;
};

struct CNetEvent_Explosion : public CNetEvent_Common
{
};

struct CNetEvent_Spawn : public CNetEvent_Common
{
};

struct CNetEvent_HammerHit : public CNetEvent_Common
{
};

struct CNetEvent_Death : public CNetEvent_Common
{
	int m_ClientID;
};

struct CNetEvent_SoundWorld : public CNetEvent_Common
{
	int m_SoundID;
};

struct CNetEvent_DamageInd : public CNetEvent_Common
{
	int m_Angle;
};

struct CNetMsg_Sv_Motd
{
	const char *m_pMessage;
	int MsgID() const { return NETMSGTYPE_SV_MOTD; }
	
	bool Pack(CMsgPacker *pPacker)
	{
		pPacker->AddString(m_pMessage, -1);
		return pPacker->Error() != 0;
	}
};

struct CNetMsg_Sv_Chat
{
	int m_Team;
	int m_ClientID;
	const char *m_pMessage;
	int MsgID() const { return NETMSGTYPE_SV_CHAT; }
	
	bool Pack(CMsgPacker *pPacker)
	{
		pPacker->AddInt(m_Team);
		pPacker->AddInt(m_ClientID);
		pPacker->AddString(m_pMessage, -1);
		return pPacker->Error() != 0;
	}
};

struct CNetMsg_Sv_Team
{
	int m_ClientID;
	int m_Team;
	int m_Silent;
	int m_CooldownTick;
	int MsgID() const { return NETMSGTYPE_SV_TEAM; }
	
	bool Pack(CMsgPacker *pPacker)
	{
		pPacker->AddInt(m_ClientID);
		pPacker->AddInt(m_Team);
		pPacker->AddInt(m_Silent);
		pPacker->AddInt(m_CooldownTick);
		return pPacker->Error() != 0;
	}
};

struct CNetMsg_Sv_KillMsg
{
	int m_Killer;
	int m_Victim;
	int m_Weapon;
	int m_ModeSpecial;
	int MsgID() const { return NETMSGTYPE_SV_KILLMSG; }
	
	bool Pack(CMsgPacker *pPacker)
	{
		pPacker->AddInt(m_Killer);
		pPacker->AddInt(m_Victim);
		pPacker->AddInt(m_Weapon);
		pPacker->AddInt(m_ModeSpecial);
		return pPacker->Error() != 0;
	}
};

struct CNetMsg_Sv_TuneParams
{
	int MsgID() const { return NETMSGTYPE_SV_TUNEPARAMS; }
	
	bool Pack(CMsgPacker *pPacker)
	{
		return pPacker->Error() != 0;
	}
};

struct CNetMsg_Sv_ExtraProjectile
{
	int MsgID() const { return NETMSGTYPE_SV_EXTRAPROJECTILE; }
	
	bool Pack(CMsgPacker *pPacker)
	{
		return pPacker->Error() != 0;
	}
};

struct CNetMsg_Sv_ReadyToEnter
{
	int MsgID() const { return NETMSGTYPE_SV_READYTOENTER; }
	
	bool Pack(CMsgPacker *pPacker)
	{
		return pPacker->Error() != 0;
	}
};

struct CNetMsg_Sv_WeaponPickup
{
	int m_Weapon;
	int MsgID() const { return NETMSGTYPE_SV_WEAPONPICKUP; }
	
	bool Pack(CMsgPacker *pPacker)
	{
		pPacker->AddInt(m_Weapon);
		return pPacker->Error() != 0;
	}
};

struct CNetMsg_Sv_Emoticon
{
	int m_ClientID;
	int m_Emoticon;
	int MsgID() const { return NETMSGTYPE_SV_EMOTICON; }
	
	bool Pack(CMsgPacker *pPacker)
	{
		pPacker->AddInt(m_ClientID);
		pPacker->AddInt(m_Emoticon);
		return pPacker->Error() != 0;
	}
};

struct CNetMsg_Sv_VoteClearOptions
{
	int MsgID() const { return NETMSGTYPE_SV_VOTECLEAROPTIONS; }
	
	bool Pack(CMsgPacker *pPacker)
	{
		return pPacker->Error() != 0;
	}
};

struct CNetMsg_Sv_VoteOptionListAdd
{
	int MsgID() const { return NETMSGTYPE_SV_VOTEOPTIONLISTADD; }
	
	bool Pack(CMsgPacker *pPacker)
	{
		return pPacker->Error() != 0;
	}
};

struct CNetMsg_Sv_VoteOptionAdd
{
	const char *m_pDescription;
	int MsgID() const { return NETMSGTYPE_SV_VOTEOPTIONADD; }
	
	bool Pack(CMsgPacker *pPacker)
	{
		pPacker->AddString(m_pDescription, -1);
		return pPacker->Error() != 0;
	}
};

struct CNetMsg_Sv_VoteOptionRemove
{
	const char *m_pDescription;
	int MsgID() const { return NETMSGTYPE_SV_VOTEOPTIONREMOVE; }
	
	bool Pack(CMsgPacker *pPacker)
	{
		pPacker->AddString(m_pDescription, -1);
		return pPacker->Error() != 0;
	}
};

struct CNetMsg_Sv_VoteSet
{
	int m_ClientID;
	int m_Type;
	int m_Timeout;
	const char *m_pDescription;
	const char *m_pReason;
	int MsgID() const { return NETMSGTYPE_SV_VOTESET; }
	
	bool Pack(CMsgPacker *pPacker)
	{
		pPacker->AddInt(m_ClientID);
		pPacker->AddInt(m_Type);
		pPacker->AddInt(m_Timeout);
		pPacker->AddString(m_pDescription, -1);
		pPacker->AddString(m_pReason, -1);
		return pPacker->Error() != 0;
	}
};

struct CNetMsg_Sv_VoteStatus
{
	int m_Yes;
	int m_No;
	int m_Pass;
	int m_Total;
	int MsgID() const { return NETMSGTYPE_SV_VOTESTATUS; }
	
	bool Pack(CMsgPacker *pPacker)
	{
		pPacker->AddInt(m_Yes);
		pPacker->AddInt(m_No);
		pPacker->AddInt(m_Pass);
		pPacker->AddInt(m_Total);
		return pPacker->Error() != 0;
	}
};

struct CNetMsg_Sv_ServerSettings
{
	int m_KickVote;
	int m_KickMin;
	int m_SpecVote;
	int m_TeamLock;
	int m_TeamBalance;
	int m_PlayerSlots;
	int MsgID() const { return NETMSGTYPE_SV_SERVERSETTINGS; }
	
	bool Pack(CMsgPacker *pPacker)
	{
		pPacker->AddInt(m_KickVote);
		pPacker->AddInt(m_KickMin);
		pPacker->AddInt(m_SpecVote);
		pPacker->AddInt(m_TeamLock);
		pPacker->AddInt(m_TeamBalance);
		pPacker->AddInt(m_PlayerSlots);
		return pPacker->Error() != 0;
	}
};

struct CNetMsg_Sv_ClientInfo
{
	int m_ClientID;
	int m_Local;
	int m_Team;
	const char *m_pName;
	const char *m_pClan;
	int m_Country;
	const char *m_apSkinPartNames[6];
	int m_aUseCustomColors[6];
	int m_aSkinPartColors[6];
	int MsgID() const { return NETMSGTYPE_SV_CLIENTINFO; }
	
	bool Pack(CMsgPacker *pPacker)
	{
		pPacker->AddInt(m_ClientID);
		pPacker->AddInt(m_Local);
		pPacker->AddInt(m_Team);
		pPacker->AddString(m_pName, -1);
		pPacker->AddString(m_pClan, -1);
		pPacker->AddInt(m_Country);
		pPacker->AddString(m_apSkinPartNames[0], -1);
		pPacker->AddString(m_apSkinPartNames[1], -1);
		pPacker->AddString(m_apSkinPartNames[2], -1);
		pPacker->AddString(m_apSkinPartNames[3], -1);
		pPacker->AddString(m_apSkinPartNames[4], -1);
		pPacker->AddString(m_apSkinPartNames[5], -1);
		pPacker->AddInt(m_aUseCustomColors[0]);
		pPacker->AddInt(m_aUseCustomColors[1]);
		pPacker->AddInt(m_aUseCustomColors[2]);
		pPacker->AddInt(m_aUseCustomColors[3]);
		pPacker->AddInt(m_aUseCustomColors[4]);
		pPacker->AddInt(m_aUseCustomColors[5]);
		pPacker->AddInt(m_aSkinPartColors[0]);
		pPacker->AddInt(m_aSkinPartColors[1]);
		pPacker->AddInt(m_aSkinPartColors[2]);
		pPacker->AddInt(m_aSkinPartColors[3]);
		pPacker->AddInt(m_aSkinPartColors[4]);
		pPacker->AddInt(m_aSkinPartColors[5]);
		return pPacker->Error() != 0;
	}
};

struct CNetMsg_Sv_GameInfo
{
	int m_GameFlags;
	int m_ScoreLimit;
	int m_TimeLimit;
	int m_MatchNum;
	int m_MatchCurrent;
	int MsgID() const { return NETMSGTYPE_SV_GAMEINFO; }
	
	bool Pack(CMsgPacker *pPacker)
	{
		pPacker->AddInt(m_GameFlags);
		pPacker->AddInt(m_ScoreLimit);
		pPacker->AddInt(m_TimeLimit);
		pPacker->AddInt(m_MatchNum);
		pPacker->AddInt(m_MatchCurrent);
		return pPacker->Error() != 0;
	}
};

struct CNetMsg_Sv_ClientDrop
{
	int m_ClientID;
	const char *m_pReason;
	int MsgID() const { return NETMSGTYPE_SV_CLIENTDROP; }
	
	bool Pack(CMsgPacker *pPacker)
	{
		pPacker->AddInt(m_ClientID);
		pPacker->AddString(m_pReason, -1);
		return pPacker->Error() != 0;
	}
};

struct CNetMsg_Sv_GameMsg
{
	int MsgID() const { return NETMSGTYPE_SV_GAMEMSG; }
	
	bool Pack(CMsgPacker *pPacker)
	{
		return pPacker->Error() != 0;
	}
};

struct CNetMsg_De_ClientEnter
{
	const char *m_pName;
	int m_Team;
	int MsgID() const { return NETMSGTYPE_DE_CLIENTENTER; }
	
	bool Pack(CMsgPacker *pPacker)
	{
		pPacker->AddString(m_pName, -1);
		pPacker->AddInt(m_Team);
		return pPacker->Error() != 0;
	}
};

struct CNetMsg_De_ClientLeave
{
	const char *m_pName;
	const char *m_pReason;
	int MsgID() const { return NETMSGTYPE_DE_CLIENTLEAVE; }
	
	bool Pack(CMsgPacker *pPacker)
	{
		pPacker->AddString(m_pName, -1);
		pPacker->AddString(m_pReason, -1);
		return pPacker->Error() != 0;
	}
};

struct CNetMsg_Cl_Say
{
	int m_Team;
	const char *m_pMessage;
	int MsgID() const { return NETMSGTYPE_CL_SAY; }
	
	bool Pack(CMsgPacker *pPacker)
	{
		pPacker->AddInt(m_Team);
		pPacker->AddString(m_pMessage, -1);
		return pPacker->Error() != 0;
	}
};

struct CNetMsg_Cl_SetTeam
{
	int m_Team;
	int MsgID() const { return NETMSGTYPE_CL_SETTEAM; }
	
	bool Pack(CMsgPacker *pPacker)
	{
		pPacker->AddInt(m_Team);
		return pPacker->Error() != 0;
	}
};

struct CNetMsg_Cl_SetSpectatorMode
{
	int m_SpectatorID;
	int MsgID() const { return NETMSGTYPE_CL_SETSPECTATORMODE; }
	
	bool Pack(CMsgPacker *pPacker)
	{
		pPacker->AddInt(m_SpectatorID);
		return pPacker->Error() != 0;
	}
};

struct CNetMsg_Cl_StartInfo
{
	const char *m_pName;
	const char *m_pClan;
	int m_Country;
	const char *m_apSkinPartNames[6];
	int m_aUseCustomColors[6];
	int m_aSkinPartColors[6];
	int MsgID() const { return NETMSGTYPE_CL_STARTINFO; }
	
	bool Pack(CMsgPacker *pPacker)
	{
		pPacker->AddString(m_pName, -1);
		pPacker->AddString(m_pClan, -1);
		pPacker->AddInt(m_Country);
		pPacker->AddString(m_apSkinPartNames[0], -1);
		pPacker->AddString(m_apSkinPartNames[1], -1);
		pPacker->AddString(m_apSkinPartNames[2], -1);
		pPacker->AddString(m_apSkinPartNames[3], -1);
		pPacker->AddString(m_apSkinPartNames[4], -1);
		pPacker->AddString(m_apSkinPartNames[5], -1);
		pPacker->AddInt(m_aUseCustomColors[0]);
		pPacker->AddInt(m_aUseCustomColors[1]);
		pPacker->AddInt(m_aUseCustomColors[2]);
		pPacker->AddInt(m_aUseCustomColors[3]);
		pPacker->AddInt(m_aUseCustomColors[4]);
		pPacker->AddInt(m_aUseCustomColors[5]);
		pPacker->AddInt(m_aSkinPartColors[0]);
		pPacker->AddInt(m_aSkinPartColors[1]);
		pPacker->AddInt(m_aSkinPartColors[2]);
		pPacker->AddInt(m_aSkinPartColors[3]);
		pPacker->AddInt(m_aSkinPartColors[4]);
		pPacker->AddInt(m_aSkinPartColors[5]);
		return pPacker->Error() != 0;
	}
};

struct CNetMsg_Cl_Kill
{
	int MsgID() const { return NETMSGTYPE_CL_KILL; }
	
	bool Pack(CMsgPacker *pPacker)
	{
		return pPacker->Error() != 0;
	}
};

struct CNetMsg_Cl_ReadyChange
{
	int MsgID() const { return NETMSGTYPE_CL_READYCHANGE; }
	
	bool Pack(CMsgPacker *pPacker)
	{
		return pPacker->Error() != 0;
	}
};

struct CNetMsg_Cl_Emoticon
{
	int m_Emoticon;
	int MsgID() const { return NETMSGTYPE_CL_EMOTICON; }
	
	bool Pack(CMsgPacker *pPacker)
	{
		pPacker->AddInt(m_Emoticon);
		return pPacker->Error() != 0;
	}
};

struct CNetMsg_Cl_Vote
{
	int m_Vote;
	int MsgID() const { return NETMSGTYPE_CL_VOTE; }
	
	bool Pack(CMsgPacker *pPacker)
	{
		pPacker->AddInt(m_Vote);
		return pPacker->Error() != 0;
	}
};

struct CNetMsg_Cl_CallVote
{
	const char *m_Type;
	const char *m_Value;
	const char *m_Reason;
	int m_Force;
	int MsgID() const { return NETMSGTYPE_CL_CALLVOTE; }
	
	bool Pack(CMsgPacker *pPacker)
	{
		pPacker->AddString(m_Type, -1);
		pPacker->AddString(m_Value, -1);
		pPacker->AddString(m_Reason, -1);
		pPacker->AddInt(m_Force);
		return pPacker->Error() != 0;
	}
};

enum
{
	SOUND_GUN_FIRE=0,
	SOUND_SHOTGUN_FIRE,
	SOUND_GRENADE_FIRE,
	SOUND_HAMMER_FIRE,
	SOUND_HAMMER_HIT,
	SOUND_NINJA_FIRE,
	SOUND_GRENADE_EXPLODE,
	SOUND_NINJA_HIT,
	SOUND_LASER_FIRE,
	SOUND_LASER_BOUNCE,
	SOUND_WEAPON_SWITCH,
	SOUND_PLAYER_PAIN_SHORT,
	SOUND_PLAYER_PAIN_LONG,
	SOUND_BODY_LAND,
	SOUND_PLAYER_AIRJUMP,
	SOUND_PLAYER_JUMP,
	SOUND_PLAYER_DIE,
	SOUND_PLAYER_SPAWN,
	SOUND_PLAYER_SKID,
	SOUND_TEE_CRY,
	SOUND_HOOK_LOOP,
	SOUND_HOOK_ATTACH_GROUND,
	SOUND_HOOK_ATTACH_PLAYER,
	SOUND_HOOK_NOATTACH,
	SOUND_PICKUP_HEALTH,
	SOUND_PICKUP_ARMOR,
	SOUND_PICKUP_GRENADE,
	SOUND_PICKUP_SHOTGUN,
	SOUND_PICKUP_NINJA,
	SOUND_WEAPON_SPAWN,
	SOUND_WEAPON_NOAMMO,
	SOUND_HIT,
	SOUND_CHAT_SERVER,
	SOUND_CHAT_CLIENT,
	SOUND_CHAT_HIGHLIGHT,
	SOUND_CTF_DROP,
	SOUND_CTF_RETURN,
	SOUND_CTF_GRAB_PL,
	SOUND_CTF_GRAB_EN,
	SOUND_CTF_CAPTURE,
	SOUND_MENU,
	NUM_SOUNDS
};
enum
{
	WEAPON_HAMMER=0,
	WEAPON_GUN,
	WEAPON_SHOTGUN,
	WEAPON_GRENADE,
	WEAPON_LASER,
	WEAPON_NINJA,
	NUM_WEAPONS
};


class CNetObjHandler
{
	const char *m_pMsgFailedOn;
	const char *m_pObjCorrectedOn;
	char m_aMsgData[1024];
	int m_NumObjCorrections;
	int ClampInt(const char *pErrorMsg, int Value, int Min, int Max);
	int ClampFlag(const char *pErrorMsg, int Value, int Mask);

	static const char *ms_apObjNames[];
	static int ms_aObjSizes[];
	static const char *ms_apMsgNames[];

public:
	CNetObjHandler();

	int ValidateObj(int Type, void *pData, int Size);
	const char *GetObjName(int Type);
	int GetObjSize(int Type);
	int NumObjCorrections();
	const char *CorrectedObjOn();

	const char *GetMsgName(int Type);
	void *SecureUnpackMsg(int Type, CUnpacker *pUnpacker);
	const char *FailedMsgOn();
};

} // namespace Protocol7

#endif // PROXY_07_PROTOCOL_H
