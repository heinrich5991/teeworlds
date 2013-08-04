#include <engine/shared/protocol.h>
#include <engine/message.h>
#include "protocol_generated.h"
namespace Protocol7
{
CNetObjHandler::CNetObjHandler()
{
	m_pMsgFailedOn = "";
	m_pObjCorrectedOn = "";
	m_NumObjCorrections = 0;
}

int CNetObjHandler::NumObjCorrections() { return m_NumObjCorrections; }
const char *CNetObjHandler::CorrectedObjOn() { return m_pObjCorrectedOn; }
const char *CNetObjHandler::FailedMsgOn() { return m_pMsgFailedOn; }





static const int max_int = 0x7fffffff;

int CNetObjHandler::ClampInt(const char *pErrorMsg, int Value, int Min, int Max)
{
	if(Value < Min) { m_pObjCorrectedOn = pErrorMsg; m_NumObjCorrections++; return Min; }
	if(Value > Max) { m_pObjCorrectedOn = pErrorMsg; m_NumObjCorrections++; return Max; }
	return Value;
}

int CNetObjHandler::ClampFlag(const char *pErrorMsg, int Value, int Mask)
{
	if((Value&Mask) != Value) { m_pObjCorrectedOn = pErrorMsg; m_NumObjCorrections++; return (Value&Mask); }
	return Value;
}

const char *CNetObjHandler::ms_apObjNames[] = {
	"invalid",
	"PlayerInput",
	"Projectile",
	"Laser",
	"Pickup",
	"Flag",
	"GameData",
	"GameDataTeam",
	"GameDataFlag",
	"CharacterCore",
	"Character",
	"PlayerInfo",
	"SpectatorInfo",
	"De_ClientInfo",
	"De_GameInfo",
	"De_TuneParams",
	"Common",
	"Explosion",
	"Spawn",
	"HammerHit",
	"Death",
	"SoundWorld",
	"DamageInd",
	""
};

int CNetObjHandler::ms_aObjSizes[] = {
	0,
	sizeof(CNetObj_PlayerInput),
	sizeof(CNetObj_Projectile),
	sizeof(CNetObj_Laser),
	sizeof(CNetObj_Pickup),
	sizeof(CNetObj_Flag),
	sizeof(CNetObj_GameData),
	sizeof(CNetObj_GameDataTeam),
	sizeof(CNetObj_GameDataFlag),
	sizeof(CNetObj_CharacterCore),
	sizeof(CNetObj_Character),
	sizeof(CNetObj_PlayerInfo),
	sizeof(CNetObj_SpectatorInfo),
	sizeof(CNetObj_De_ClientInfo),
	sizeof(CNetObj_De_GameInfo),
	sizeof(CNetObj_De_TuneParams),
	sizeof(CNetEvent_Common),
	sizeof(CNetEvent_Explosion),
	sizeof(CNetEvent_Spawn),
	sizeof(CNetEvent_HammerHit),
	sizeof(CNetEvent_Death),
	sizeof(CNetEvent_SoundWorld),
	sizeof(CNetEvent_DamageInd),
	0
};

const char *CNetObjHandler::ms_apMsgNames[] = {
	"invalid",
	"Sv_Motd",
	"Sv_Chat",
	"Sv_Team",
	"Sv_KillMsg",
	"Sv_TuneParams",
	"Sv_ExtraProjectile",
	"Sv_ReadyToEnter",
	"Sv_WeaponPickup",
	"Sv_Emoticon",
	"Sv_VoteClearOptions",
	"Sv_VoteOptionListAdd",
	"Sv_VoteOptionAdd",
	"Sv_VoteOptionRemove",
	"Sv_VoteSet",
	"Sv_VoteStatus",
	"Sv_ServerSettings",
	"Sv_ClientInfo",
	"Sv_GameInfo",
	"Sv_ClientDrop",
	"Sv_GameMsg",
	"De_ClientEnter",
	"De_ClientLeave",
	"Cl_Say",
	"Cl_SetTeam",
	"Cl_SetSpectatorMode",
	"Cl_StartInfo",
	"Cl_Kill",
	"Cl_ReadyChange",
	"Cl_Emoticon",
	"Cl_Vote",
	"Cl_CallVote",
	""
};

const char *CNetObjHandler::GetObjName(int Type)
{
	if(Type < 0 || Type >= NUM_NETOBJTYPES) return "(out of range)";
	return ms_apObjNames[Type];
};

int CNetObjHandler::GetObjSize(int Type)
{
	if(Type < 0 || Type >= NUM_NETOBJTYPES) return 0;
	return ms_aObjSizes[Type];
};

const char *CNetObjHandler::GetMsgName(int Type)
{
	if(Type < 0 || Type >= NUM_NETMSGTYPES) return "(out of range)";
	return ms_apMsgNames[Type];
};

int CNetObjHandler::ValidateObj(int Type, void *pData, int Size)
{
	switch(Type)
	{
	case NETOBJTYPE_PLAYERINPUT:
	{
		CNetObj_PlayerInput *pObj = (CNetObj_PlayerInput *)pData;
		if(sizeof(*pObj) != Size) return -1;
		ClampInt("m_Direction", pObj->m_Direction, -1, 1);
		ClampInt("m_Jump", pObj->m_Jump, 0, 1);
		ClampInt("m_Hook", pObj->m_Hook, 0, 1);
		ClampFlag("m_PlayerFlags", pObj->m_PlayerFlags, PLAYERFLAG_ADMIN|PLAYERFLAG_CHATTING|PLAYERFLAG_SCOREBOARD|PLAYERFLAG_READY|PLAYERFLAG_DEAD|PLAYERFLAG_WATCHING);
		ClampInt("m_WantedWeapon", pObj->m_WantedWeapon, 0, NUM_WEAPONS-1);
		return 0;
	}
	
	case NETOBJTYPE_PROJECTILE:
	{
		CNetObj_Projectile *pObj = (CNetObj_Projectile *)pData;
		if(sizeof(*pObj) != Size) return -1;
		ClampInt("m_Type", pObj->m_Type, 0, NUM_WEAPONS-1);
		ClampInt("m_StartTick", pObj->m_StartTick, 0, max_int);
		return 0;
	}
	
	case NETOBJTYPE_LASER:
	{
		CNetObj_Laser *pObj = (CNetObj_Laser *)pData;
		if(sizeof(*pObj) != Size) return -1;
		ClampInt("m_StartTick", pObj->m_StartTick, 0, max_int);
		return 0;
	}
	
	case NETOBJTYPE_PICKUP:
	{
		CNetObj_Pickup *pObj = (CNetObj_Pickup *)pData;
		if(sizeof(*pObj) != Size) return -1;
		ClampInt("m_Type", pObj->m_Type, 0, 6);
		return 0;
	}
	
	case NETOBJTYPE_FLAG:
	{
		CNetObj_Flag *pObj = (CNetObj_Flag *)pData;
		if(sizeof(*pObj) != Size) return -1;
		ClampInt("m_Team", pObj->m_Team, TEAM_RED, TEAM_BLUE);
		return 0;
	}
	
	case NETOBJTYPE_GAMEDATA:
	{
		CNetObj_GameData *pObj = (CNetObj_GameData *)pData;
		if(sizeof(*pObj) != Size) return -1;
		ClampInt("m_GameStartTick", pObj->m_GameStartTick, 0, max_int);
		ClampFlag("m_GameStateFlags", pObj->m_GameStateFlags, GAMESTATEFLAG_WARMUP|GAMESTATEFLAG_SUDDENDEATH|GAMESTATEFLAG_ROUNDOVER|GAMESTATEFLAG_GAMEOVER|GAMESTATEFLAG_PAUSED|GAMESTATEFLAG_STARTCOUNTDOWN);
		ClampInt("m_GameStateEndTick", pObj->m_GameStateEndTick, 0, max_int);
		return 0;
	}
	
	case NETOBJTYPE_GAMEDATATEAM:
	{
		CNetObj_GameDataTeam *pObj = (CNetObj_GameDataTeam *)pData;
		if(sizeof(*pObj) != Size) return -1;
		return 0;
	}
	
	case NETOBJTYPE_GAMEDATAFLAG:
	{
		CNetObj_GameDataFlag *pObj = (CNetObj_GameDataFlag *)pData;
		if(sizeof(*pObj) != Size) return -1;
		ClampInt("m_FlagCarrierRed", pObj->m_FlagCarrierRed, FLAG_MISSING, MAX_CLIENTS-1);
		ClampInt("m_FlagCarrierBlue", pObj->m_FlagCarrierBlue, FLAG_MISSING, MAX_CLIENTS-1);
		ClampInt("m_FlagDropTickRed", pObj->m_FlagDropTickRed, 0, max_int);
		ClampInt("m_FlagDropTickBlue", pObj->m_FlagDropTickBlue, 0, max_int);
		return 0;
	}
	
	case NETOBJTYPE_CHARACTERCORE:
	{
		CNetObj_CharacterCore *pObj = (CNetObj_CharacterCore *)pData;
		if(sizeof(*pObj) != Size) return -1;
		ClampInt("m_Tick", pObj->m_Tick, 0, max_int);
		ClampInt("m_Direction", pObj->m_Direction, -1, 1);
		ClampInt("m_Jumped", pObj->m_Jumped, 0, 3);
		ClampInt("m_HookedPlayer", pObj->m_HookedPlayer, 0, MAX_CLIENTS-1);
		ClampInt("m_HookState", pObj->m_HookState, -1, 5);
		ClampInt("m_HookTick", pObj->m_HookTick, 0, max_int);
		return 0;
	}
	
	case NETOBJTYPE_CHARACTER:
	{
		CNetObj_Character *pObj = (CNetObj_Character *)pData;
		if(sizeof(*pObj) != Size) return -1;
		ClampInt("m_Health", pObj->m_Health, 0, 10);
		ClampInt("m_Armor", pObj->m_Armor, 0, 10);
		ClampInt("m_Weapon", pObj->m_Weapon, 0, NUM_WEAPONS-1);
		ClampInt("m_Emote", pObj->m_Emote, 0, 6);
		ClampInt("m_AttackTick", pObj->m_AttackTick, 0, max_int);
		ClampFlag("m_TriggeredEvents", pObj->m_TriggeredEvents, COREEVENTFLAG_GROUND_JUMP|COREEVENTFLAG_AIR_JUMP|COREEVENTFLAG_HOOK_ATTACH_PLAYER|COREEVENTFLAG_HOOK_ATTACH_GROUND|COREEVENTFLAG_HOOK_HIT_NOHOOK);
		return 0;
	}
	
	case NETOBJTYPE_PLAYERINFO:
	{
		CNetObj_PlayerInfo *pObj = (CNetObj_PlayerInfo *)pData;
		if(sizeof(*pObj) != Size) return -1;
		ClampFlag("m_PlayerFlags", pObj->m_PlayerFlags, PLAYERFLAG_ADMIN|PLAYERFLAG_CHATTING|PLAYERFLAG_SCOREBOARD|PLAYERFLAG_READY|PLAYERFLAG_DEAD|PLAYERFLAG_WATCHING);
		return 0;
	}
	
	case NETOBJTYPE_SPECTATORINFO:
	{
		CNetObj_SpectatorInfo *pObj = (CNetObj_SpectatorInfo *)pData;
		if(sizeof(*pObj) != Size) return -1;
		ClampInt("m_SpectatorID", pObj->m_SpectatorID, SPEC_FREEVIEW, MAX_CLIENTS-1);
		return 0;
	}
	
	case NETOBJTYPE_DE_CLIENTINFO:
	{
		CNetObj_De_ClientInfo *pObj = (CNetObj_De_ClientInfo *)pData;
		if(sizeof(*pObj) != Size) return -1;
		ClampInt("m_Local", pObj->m_Local, 0, 1);
		ClampInt("m_Team", pObj->m_Team, TEAM_SPECTATORS, TEAM_BLUE);
		ClampInt("m_aUseCustomColors[0]", pObj->m_aUseCustomColors[0], 0, 1);
		ClampInt("m_aUseCustomColors[1]", pObj->m_aUseCustomColors[1], 0, 1);
		ClampInt("m_aUseCustomColors[2]", pObj->m_aUseCustomColors[2], 0, 1);
		ClampInt("m_aUseCustomColors[3]", pObj->m_aUseCustomColors[3], 0, 1);
		ClampInt("m_aUseCustomColors[4]", pObj->m_aUseCustomColors[4], 0, 1);
		ClampInt("m_aUseCustomColors[5]", pObj->m_aUseCustomColors[5], 0, 1);
		return 0;
	}
	
	case NETOBJTYPE_DE_GAMEINFO:
	{
		CNetObj_De_GameInfo *pObj = (CNetObj_De_GameInfo *)pData;
		if(sizeof(*pObj) != Size) return -1;
		ClampFlag("m_GameFlags", pObj->m_GameFlags, GAMEFLAG_TEAMS|GAMEFLAG_FLAGS|GAMEFLAG_SURVIVAL);
		ClampInt("m_ScoreLimit", pObj->m_ScoreLimit, 0, max_int);
		ClampInt("m_TimeLimit", pObj->m_TimeLimit, 0, max_int);
		ClampInt("m_MatchNum", pObj->m_MatchNum, 0, max_int);
		ClampInt("m_MatchCurrent", pObj->m_MatchCurrent, 0, max_int);
		return 0;
	}
	
	case NETOBJTYPE_DE_TUNEPARAMS:
	{
		CNetObj_De_TuneParams *pObj = (CNetObj_De_TuneParams *)pData;
		if(sizeof(*pObj) != Size) return -1;
		return 0;
	}
	
	case NETEVENTTYPE_COMMON:
	{
		CNetEvent_Common *pObj = (CNetEvent_Common *)pData;
		if(sizeof(*pObj) != Size) return -1;
		return 0;
	}
	
	case NETEVENTTYPE_EXPLOSION:
	{
		CNetEvent_Explosion *pObj = (CNetEvent_Explosion *)pData;
		if(sizeof(*pObj) != Size) return -1;
		return 0;
	}
	
	case NETEVENTTYPE_SPAWN:
	{
		CNetEvent_Spawn *pObj = (CNetEvent_Spawn *)pData;
		if(sizeof(*pObj) != Size) return -1;
		return 0;
	}
	
	case NETEVENTTYPE_HAMMERHIT:
	{
		CNetEvent_HammerHit *pObj = (CNetEvent_HammerHit *)pData;
		if(sizeof(*pObj) != Size) return -1;
		return 0;
	}
	
	case NETEVENTTYPE_DEATH:
	{
		CNetEvent_Death *pObj = (CNetEvent_Death *)pData;
		if(sizeof(*pObj) != Size) return -1;
		ClampInt("m_ClientID", pObj->m_ClientID, 0, MAX_CLIENTS-1);
		return 0;
	}
	
	case NETEVENTTYPE_SOUNDWORLD:
	{
		CNetEvent_SoundWorld *pObj = (CNetEvent_SoundWorld *)pData;
		if(sizeof(*pObj) != Size) return -1;
		ClampInt("m_SoundID", pObj->m_SoundID, 0, NUM_SOUNDS-1);
		return 0;
	}
	
	case NETEVENTTYPE_DAMAGEIND:
	{
		CNetEvent_DamageInd *pObj = (CNetEvent_DamageInd *)pData;
		if(sizeof(*pObj) != Size) return -1;
		return 0;
	}
	
	}
	return -1;
};

void *CNetObjHandler::SecureUnpackMsg(int Type, CUnpacker *pUnpacker)
{
	m_pMsgFailedOn = 0;
	switch(Type)
	{
	case NETMSGTYPE_SV_MOTD:
	{
		CNetMsg_Sv_Motd *pMsg = (CNetMsg_Sv_Motd *)m_aMsgData;
		(void)pMsg;
		pMsg->m_pMessage = pUnpacker->GetString();
	} break;
	
	case NETMSGTYPE_SV_CHAT:
	{
		CNetMsg_Sv_Chat *pMsg = (CNetMsg_Sv_Chat *)m_aMsgData;
		(void)pMsg;
		pMsg->m_Team = pUnpacker->GetInt();
		pMsg->m_ClientID = pUnpacker->GetInt();
		pMsg->m_pMessage = pUnpacker->GetString();
		if(pMsg->m_Team < TEAM_SPECTATORS || pMsg->m_Team > TEAM_BLUE) { m_pMsgFailedOn = "m_Team"; break; }
		if(pMsg->m_ClientID < -1 || pMsg->m_ClientID > MAX_CLIENTS-1) { m_pMsgFailedOn = "m_ClientID"; break; }
	} break;
	
	case NETMSGTYPE_SV_TEAM:
	{
		CNetMsg_Sv_Team *pMsg = (CNetMsg_Sv_Team *)m_aMsgData;
		(void)pMsg;
		pMsg->m_ClientID = pUnpacker->GetInt();
		pMsg->m_Team = pUnpacker->GetInt();
		pMsg->m_Silent = pUnpacker->GetInt();
		pMsg->m_CooldownTick = pUnpacker->GetInt();
		if(pMsg->m_ClientID < -1 || pMsg->m_ClientID > MAX_CLIENTS-1) { m_pMsgFailedOn = "m_ClientID"; break; }
		if(pMsg->m_Team < TEAM_SPECTATORS || pMsg->m_Team > TEAM_BLUE) { m_pMsgFailedOn = "m_Team"; break; }
		if(pMsg->m_Silent < 0 || pMsg->m_Silent > 1) { m_pMsgFailedOn = "m_Silent"; break; }
		if(pMsg->m_CooldownTick < 0 || pMsg->m_CooldownTick > max_int) { m_pMsgFailedOn = "m_CooldownTick"; break; }
	} break;
	
	case NETMSGTYPE_SV_KILLMSG:
	{
		CNetMsg_Sv_KillMsg *pMsg = (CNetMsg_Sv_KillMsg *)m_aMsgData;
		(void)pMsg;
		pMsg->m_Killer = pUnpacker->GetInt();
		pMsg->m_Victim = pUnpacker->GetInt();
		pMsg->m_Weapon = pUnpacker->GetInt();
		pMsg->m_ModeSpecial = pUnpacker->GetInt();
		if(pMsg->m_Killer < 0 || pMsg->m_Killer > MAX_CLIENTS-1) { m_pMsgFailedOn = "m_Killer"; break; }
		if(pMsg->m_Victim < 0 || pMsg->m_Victim > MAX_CLIENTS-1) { m_pMsgFailedOn = "m_Victim"; break; }
		if(pMsg->m_Weapon < -3 || pMsg->m_Weapon > NUM_WEAPONS-1) { m_pMsgFailedOn = "m_Weapon"; break; }
	} break;
	
	case NETMSGTYPE_SV_TUNEPARAMS:
	{
		CNetMsg_Sv_TuneParams *pMsg = (CNetMsg_Sv_TuneParams *)m_aMsgData;
		(void)pMsg;
	} break;
	
	case NETMSGTYPE_SV_EXTRAPROJECTILE:
	{
		CNetMsg_Sv_ExtraProjectile *pMsg = (CNetMsg_Sv_ExtraProjectile *)m_aMsgData;
		(void)pMsg;
	} break;
	
	case NETMSGTYPE_SV_READYTOENTER:
	{
		CNetMsg_Sv_ReadyToEnter *pMsg = (CNetMsg_Sv_ReadyToEnter *)m_aMsgData;
		(void)pMsg;
	} break;
	
	case NETMSGTYPE_SV_WEAPONPICKUP:
	{
		CNetMsg_Sv_WeaponPickup *pMsg = (CNetMsg_Sv_WeaponPickup *)m_aMsgData;
		(void)pMsg;
		pMsg->m_Weapon = pUnpacker->GetInt();
		if(pMsg->m_Weapon < 0 || pMsg->m_Weapon > NUM_WEAPONS-1) { m_pMsgFailedOn = "m_Weapon"; break; }
	} break;
	
	case NETMSGTYPE_SV_EMOTICON:
	{
		CNetMsg_Sv_Emoticon *pMsg = (CNetMsg_Sv_Emoticon *)m_aMsgData;
		(void)pMsg;
		pMsg->m_ClientID = pUnpacker->GetInt();
		pMsg->m_Emoticon = pUnpacker->GetInt();
		if(pMsg->m_ClientID < 0 || pMsg->m_ClientID > MAX_CLIENTS-1) { m_pMsgFailedOn = "m_ClientID"; break; }
		if(pMsg->m_Emoticon < 0 || pMsg->m_Emoticon > 16) { m_pMsgFailedOn = "m_Emoticon"; break; }
	} break;
	
	case NETMSGTYPE_SV_VOTECLEAROPTIONS:
	{
		CNetMsg_Sv_VoteClearOptions *pMsg = (CNetMsg_Sv_VoteClearOptions *)m_aMsgData;
		(void)pMsg;
	} break;
	
	case NETMSGTYPE_SV_VOTEOPTIONLISTADD:
	{
		CNetMsg_Sv_VoteOptionListAdd *pMsg = (CNetMsg_Sv_VoteOptionListAdd *)m_aMsgData;
		(void)pMsg;
	} break;
	
	case NETMSGTYPE_SV_VOTEOPTIONADD:
	{
		CNetMsg_Sv_VoteOptionAdd *pMsg = (CNetMsg_Sv_VoteOptionAdd *)m_aMsgData;
		(void)pMsg;
		pMsg->m_pDescription = pUnpacker->GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES);
	} break;
	
	case NETMSGTYPE_SV_VOTEOPTIONREMOVE:
	{
		CNetMsg_Sv_VoteOptionRemove *pMsg = (CNetMsg_Sv_VoteOptionRemove *)m_aMsgData;
		(void)pMsg;
		pMsg->m_pDescription = pUnpacker->GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES);
	} break;
	
	case NETMSGTYPE_SV_VOTESET:
	{
		CNetMsg_Sv_VoteSet *pMsg = (CNetMsg_Sv_VoteSet *)m_aMsgData;
		(void)pMsg;
		pMsg->m_ClientID = pUnpacker->GetInt();
		pMsg->m_Type = pUnpacker->GetInt();
		pMsg->m_Timeout = pUnpacker->GetInt();
		pMsg->m_pDescription = pUnpacker->GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES);
		pMsg->m_pReason = pUnpacker->GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES);
		if(pMsg->m_ClientID < -1 || pMsg->m_ClientID > MAX_CLIENTS-1) { m_pMsgFailedOn = "m_ClientID"; break; }
		if(pMsg->m_Type < 0 || pMsg->m_Type > 7) { m_pMsgFailedOn = "m_Type"; break; }
		if(pMsg->m_Timeout < 0 || pMsg->m_Timeout > 60) { m_pMsgFailedOn = "m_Timeout"; break; }
	} break;
	
	case NETMSGTYPE_SV_VOTESTATUS:
	{
		CNetMsg_Sv_VoteStatus *pMsg = (CNetMsg_Sv_VoteStatus *)m_aMsgData;
		(void)pMsg;
		pMsg->m_Yes = pUnpacker->GetInt();
		pMsg->m_No = pUnpacker->GetInt();
		pMsg->m_Pass = pUnpacker->GetInt();
		pMsg->m_Total = pUnpacker->GetInt();
		if(pMsg->m_Yes < 0 || pMsg->m_Yes > MAX_CLIENTS) { m_pMsgFailedOn = "m_Yes"; break; }
		if(pMsg->m_No < 0 || pMsg->m_No > MAX_CLIENTS) { m_pMsgFailedOn = "m_No"; break; }
		if(pMsg->m_Pass < 0 || pMsg->m_Pass > MAX_CLIENTS) { m_pMsgFailedOn = "m_Pass"; break; }
		if(pMsg->m_Total < 0 || pMsg->m_Total > MAX_CLIENTS) { m_pMsgFailedOn = "m_Total"; break; }
	} break;
	
	case NETMSGTYPE_SV_SERVERSETTINGS:
	{
		CNetMsg_Sv_ServerSettings *pMsg = (CNetMsg_Sv_ServerSettings *)m_aMsgData;
		(void)pMsg;
		pMsg->m_KickVote = pUnpacker->GetInt();
		pMsg->m_KickMin = pUnpacker->GetInt();
		pMsg->m_SpecVote = pUnpacker->GetInt();
		pMsg->m_TeamLock = pUnpacker->GetInt();
		pMsg->m_TeamBalance = pUnpacker->GetInt();
		pMsg->m_PlayerSlots = pUnpacker->GetInt();
		if(pMsg->m_KickVote < 0 || pMsg->m_KickVote > 1) { m_pMsgFailedOn = "m_KickVote"; break; }
		if(pMsg->m_KickMin < 0 || pMsg->m_KickMin > MAX_CLIENTS) { m_pMsgFailedOn = "m_KickMin"; break; }
		if(pMsg->m_SpecVote < 0 || pMsg->m_SpecVote > 1) { m_pMsgFailedOn = "m_SpecVote"; break; }
		if(pMsg->m_TeamLock < 0 || pMsg->m_TeamLock > 1) { m_pMsgFailedOn = "m_TeamLock"; break; }
		if(pMsg->m_TeamBalance < 0 || pMsg->m_TeamBalance > 1) { m_pMsgFailedOn = "m_TeamBalance"; break; }
		if(pMsg->m_PlayerSlots < 0 || pMsg->m_PlayerSlots > MAX_CLIENTS) { m_pMsgFailedOn = "m_PlayerSlots"; break; }
	} break;
	
	case NETMSGTYPE_SV_CLIENTINFO:
	{
		CNetMsg_Sv_ClientInfo *pMsg = (CNetMsg_Sv_ClientInfo *)m_aMsgData;
		(void)pMsg;
		pMsg->m_ClientID = pUnpacker->GetInt();
		pMsg->m_Local = pUnpacker->GetInt();
		pMsg->m_Team = pUnpacker->GetInt();
		pMsg->m_pName = pUnpacker->GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES);
		pMsg->m_pClan = pUnpacker->GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES);
		pMsg->m_Country = pUnpacker->GetInt();
		pMsg->m_apSkinPartNames[0] = pUnpacker->GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES);
		pMsg->m_apSkinPartNames[1] = pUnpacker->GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES);
		pMsg->m_apSkinPartNames[2] = pUnpacker->GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES);
		pMsg->m_apSkinPartNames[3] = pUnpacker->GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES);
		pMsg->m_apSkinPartNames[4] = pUnpacker->GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES);
		pMsg->m_apSkinPartNames[5] = pUnpacker->GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES);
		pMsg->m_aUseCustomColors[0] = pUnpacker->GetInt();
		pMsg->m_aUseCustomColors[1] = pUnpacker->GetInt();
		pMsg->m_aUseCustomColors[2] = pUnpacker->GetInt();
		pMsg->m_aUseCustomColors[3] = pUnpacker->GetInt();
		pMsg->m_aUseCustomColors[4] = pUnpacker->GetInt();
		pMsg->m_aUseCustomColors[5] = pUnpacker->GetInt();
		pMsg->m_aSkinPartColors[0] = pUnpacker->GetInt();
		pMsg->m_aSkinPartColors[1] = pUnpacker->GetInt();
		pMsg->m_aSkinPartColors[2] = pUnpacker->GetInt();
		pMsg->m_aSkinPartColors[3] = pUnpacker->GetInt();
		pMsg->m_aSkinPartColors[4] = pUnpacker->GetInt();
		pMsg->m_aSkinPartColors[5] = pUnpacker->GetInt();
		if(pMsg->m_ClientID < 0 || pMsg->m_ClientID > MAX_CLIENTS-1) { m_pMsgFailedOn = "m_ClientID"; break; }
		if(pMsg->m_Local < 0 || pMsg->m_Local > 1) { m_pMsgFailedOn = "m_Local"; break; }
		if(pMsg->m_Team < TEAM_SPECTATORS || pMsg->m_Team > TEAM_BLUE) { m_pMsgFailedOn = "m_Team"; break; }
		if(pMsg->m_aUseCustomColors[0] < 0 || pMsg->m_aUseCustomColors[0] > 1) { m_pMsgFailedOn = "m_aUseCustomColors[0]"; break; }
		if(pMsg->m_aUseCustomColors[1] < 0 || pMsg->m_aUseCustomColors[1] > 1) { m_pMsgFailedOn = "m_aUseCustomColors[1]"; break; }
		if(pMsg->m_aUseCustomColors[2] < 0 || pMsg->m_aUseCustomColors[2] > 1) { m_pMsgFailedOn = "m_aUseCustomColors[2]"; break; }
		if(pMsg->m_aUseCustomColors[3] < 0 || pMsg->m_aUseCustomColors[3] > 1) { m_pMsgFailedOn = "m_aUseCustomColors[3]"; break; }
		if(pMsg->m_aUseCustomColors[4] < 0 || pMsg->m_aUseCustomColors[4] > 1) { m_pMsgFailedOn = "m_aUseCustomColors[4]"; break; }
		if(pMsg->m_aUseCustomColors[5] < 0 || pMsg->m_aUseCustomColors[5] > 1) { m_pMsgFailedOn = "m_aUseCustomColors[5]"; break; }
	} break;
	
	case NETMSGTYPE_SV_GAMEINFO:
	{
		CNetMsg_Sv_GameInfo *pMsg = (CNetMsg_Sv_GameInfo *)m_aMsgData;
		(void)pMsg;
		pMsg->m_GameFlags = pUnpacker->GetInt();
		pMsg->m_ScoreLimit = pUnpacker->GetInt();
		pMsg->m_TimeLimit = pUnpacker->GetInt();
		pMsg->m_MatchNum = pUnpacker->GetInt();
		pMsg->m_MatchCurrent = pUnpacker->GetInt();
		if((pMsg->m_GameFlags & (GAMEFLAG_TEAMS|GAMEFLAG_FLAGS|GAMEFLAG_SURVIVAL)) != pMsg->m_GameFlags) { m_pMsgFailedOn = "m_GameFlags"; break; }
		if(pMsg->m_ScoreLimit < 0 || pMsg->m_ScoreLimit > max_int) { m_pMsgFailedOn = "m_ScoreLimit"; break; }
		if(pMsg->m_TimeLimit < 0 || pMsg->m_TimeLimit > max_int) { m_pMsgFailedOn = "m_TimeLimit"; break; }
		if(pMsg->m_MatchNum < 0 || pMsg->m_MatchNum > max_int) { m_pMsgFailedOn = "m_MatchNum"; break; }
		if(pMsg->m_MatchCurrent < 0 || pMsg->m_MatchCurrent > max_int) { m_pMsgFailedOn = "m_MatchCurrent"; break; }
	} break;
	
	case NETMSGTYPE_SV_CLIENTDROP:
	{
		CNetMsg_Sv_ClientDrop *pMsg = (CNetMsg_Sv_ClientDrop *)m_aMsgData;
		(void)pMsg;
		pMsg->m_ClientID = pUnpacker->GetInt();
		pMsg->m_pReason = pUnpacker->GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES);
		if(pMsg->m_ClientID < 0 || pMsg->m_ClientID > MAX_CLIENTS-1) { m_pMsgFailedOn = "m_ClientID"; break; }
	} break;
	
	case NETMSGTYPE_SV_GAMEMSG:
	{
		CNetMsg_Sv_GameMsg *pMsg = (CNetMsg_Sv_GameMsg *)m_aMsgData;
		(void)pMsg;
	} break;
	
	case NETMSGTYPE_DE_CLIENTENTER:
	{
		CNetMsg_De_ClientEnter *pMsg = (CNetMsg_De_ClientEnter *)m_aMsgData;
		(void)pMsg;
		pMsg->m_pName = pUnpacker->GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES);
		pMsg->m_Team = pUnpacker->GetInt();
		if(pMsg->m_Team < TEAM_SPECTATORS || pMsg->m_Team > TEAM_BLUE) { m_pMsgFailedOn = "m_Team"; break; }
	} break;
	
	case NETMSGTYPE_DE_CLIENTLEAVE:
	{
		CNetMsg_De_ClientLeave *pMsg = (CNetMsg_De_ClientLeave *)m_aMsgData;
		(void)pMsg;
		pMsg->m_pName = pUnpacker->GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES);
		pMsg->m_pReason = pUnpacker->GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES);
	} break;
	
	case NETMSGTYPE_CL_SAY:
	{
		CNetMsg_Cl_Say *pMsg = (CNetMsg_Cl_Say *)m_aMsgData;
		(void)pMsg;
		pMsg->m_Team = pUnpacker->GetInt();
		pMsg->m_pMessage = pUnpacker->GetString();
		if(pMsg->m_Team < 0 || pMsg->m_Team > 1) { m_pMsgFailedOn = "m_Team"; break; }
	} break;
	
	case NETMSGTYPE_CL_SETTEAM:
	{
		CNetMsg_Cl_SetTeam *pMsg = (CNetMsg_Cl_SetTeam *)m_aMsgData;
		(void)pMsg;
		pMsg->m_Team = pUnpacker->GetInt();
		if(pMsg->m_Team < TEAM_SPECTATORS || pMsg->m_Team > TEAM_BLUE) { m_pMsgFailedOn = "m_Team"; break; }
	} break;
	
	case NETMSGTYPE_CL_SETSPECTATORMODE:
	{
		CNetMsg_Cl_SetSpectatorMode *pMsg = (CNetMsg_Cl_SetSpectatorMode *)m_aMsgData;
		(void)pMsg;
		pMsg->m_SpectatorID = pUnpacker->GetInt();
		if(pMsg->m_SpectatorID < SPEC_FREEVIEW || pMsg->m_SpectatorID > MAX_CLIENTS-1) { m_pMsgFailedOn = "m_SpectatorID"; break; }
	} break;
	
	case NETMSGTYPE_CL_STARTINFO:
	{
		CNetMsg_Cl_StartInfo *pMsg = (CNetMsg_Cl_StartInfo *)m_aMsgData;
		(void)pMsg;
		pMsg->m_pName = pUnpacker->GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES);
		pMsg->m_pClan = pUnpacker->GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES);
		pMsg->m_Country = pUnpacker->GetInt();
		pMsg->m_apSkinPartNames[0] = pUnpacker->GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES);
		pMsg->m_apSkinPartNames[1] = pUnpacker->GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES);
		pMsg->m_apSkinPartNames[2] = pUnpacker->GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES);
		pMsg->m_apSkinPartNames[3] = pUnpacker->GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES);
		pMsg->m_apSkinPartNames[4] = pUnpacker->GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES);
		pMsg->m_apSkinPartNames[5] = pUnpacker->GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES);
		pMsg->m_aUseCustomColors[0] = pUnpacker->GetInt();
		pMsg->m_aUseCustomColors[1] = pUnpacker->GetInt();
		pMsg->m_aUseCustomColors[2] = pUnpacker->GetInt();
		pMsg->m_aUseCustomColors[3] = pUnpacker->GetInt();
		pMsg->m_aUseCustomColors[4] = pUnpacker->GetInt();
		pMsg->m_aUseCustomColors[5] = pUnpacker->GetInt();
		pMsg->m_aSkinPartColors[0] = pUnpacker->GetInt();
		pMsg->m_aSkinPartColors[1] = pUnpacker->GetInt();
		pMsg->m_aSkinPartColors[2] = pUnpacker->GetInt();
		pMsg->m_aSkinPartColors[3] = pUnpacker->GetInt();
		pMsg->m_aSkinPartColors[4] = pUnpacker->GetInt();
		pMsg->m_aSkinPartColors[5] = pUnpacker->GetInt();
		if(pMsg->m_aUseCustomColors[0] < 0 || pMsg->m_aUseCustomColors[0] > 1) { m_pMsgFailedOn = "m_aUseCustomColors[0]"; break; }
		if(pMsg->m_aUseCustomColors[1] < 0 || pMsg->m_aUseCustomColors[1] > 1) { m_pMsgFailedOn = "m_aUseCustomColors[1]"; break; }
		if(pMsg->m_aUseCustomColors[2] < 0 || pMsg->m_aUseCustomColors[2] > 1) { m_pMsgFailedOn = "m_aUseCustomColors[2]"; break; }
		if(pMsg->m_aUseCustomColors[3] < 0 || pMsg->m_aUseCustomColors[3] > 1) { m_pMsgFailedOn = "m_aUseCustomColors[3]"; break; }
		if(pMsg->m_aUseCustomColors[4] < 0 || pMsg->m_aUseCustomColors[4] > 1) { m_pMsgFailedOn = "m_aUseCustomColors[4]"; break; }
		if(pMsg->m_aUseCustomColors[5] < 0 || pMsg->m_aUseCustomColors[5] > 1) { m_pMsgFailedOn = "m_aUseCustomColors[5]"; break; }
	} break;
	
	case NETMSGTYPE_CL_KILL:
	{
		CNetMsg_Cl_Kill *pMsg = (CNetMsg_Cl_Kill *)m_aMsgData;
		(void)pMsg;
	} break;
	
	case NETMSGTYPE_CL_READYCHANGE:
	{
		CNetMsg_Cl_ReadyChange *pMsg = (CNetMsg_Cl_ReadyChange *)m_aMsgData;
		(void)pMsg;
	} break;
	
	case NETMSGTYPE_CL_EMOTICON:
	{
		CNetMsg_Cl_Emoticon *pMsg = (CNetMsg_Cl_Emoticon *)m_aMsgData;
		(void)pMsg;
		pMsg->m_Emoticon = pUnpacker->GetInt();
		if(pMsg->m_Emoticon < 0 || pMsg->m_Emoticon > 16) { m_pMsgFailedOn = "m_Emoticon"; break; }
	} break;
	
	case NETMSGTYPE_CL_VOTE:
	{
		CNetMsg_Cl_Vote *pMsg = (CNetMsg_Cl_Vote *)m_aMsgData;
		(void)pMsg;
		pMsg->m_Vote = pUnpacker->GetInt();
		if(pMsg->m_Vote < -1 || pMsg->m_Vote > 1) { m_pMsgFailedOn = "m_Vote"; break; }
	} break;
	
	case NETMSGTYPE_CL_CALLVOTE:
	{
		CNetMsg_Cl_CallVote *pMsg = (CNetMsg_Cl_CallVote *)m_aMsgData;
		(void)pMsg;
		pMsg->m_Type = pUnpacker->GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES);
		pMsg->m_Value = pUnpacker->GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES);
		pMsg->m_Reason = pUnpacker->GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES);
		pMsg->m_Force = pUnpacker->GetInt();
		if(pMsg->m_Force < 0 || pMsg->m_Force > 1) { m_pMsgFailedOn = "m_Force"; break; }
	} break;
	
	default:
		m_pMsgFailedOn = "(type out of range)";
		break;
	}
	
	if(pUnpacker->Error())
		m_pMsgFailedOn = "(unpack error)";
	
	if(m_pMsgFailedOn)
		return 0;
	m_pMsgFailedOn = "";
	return m_aMsgData;
};
} // namespace Protocol7

