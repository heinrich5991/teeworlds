// copyright (c) 2007 magnus auvinen, see licence.txt for more info
#include <engine/shared/config.h>
#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/entities/flag.h>
#include <game/server/entities/pickup.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>
#include "fastcap.h"

CGameControllerFC::CGameControllerFC(class CGameContext *pGameServer)
: CGameControllerNoTeamRace(pGameServer)
{
	m_apFlags[0] = 0;
	m_apFlags[1] = 0;
	for(int i = 0; i < MAX_CLIENTS; i++)
		m_apPlFlags[i] = 0;
	m_pGameType = "FastCap";
	m_GameFlags = GAMEFLAG_TEAMS|GAMEFLAG_FLAGS;
}

bool CGameControllerFC::OnEntity(int Index, vec2 Pos)
{
	if(IGameController::OnEntity(Index, Pos))
		return true;
	
	int Team = -1;
	if(Index == ENTITY_FLAGSTAND_RED) Team = TEAM_RED;
	if(Index == ENTITY_FLAGSTAND_BLUE) Team = TEAM_BLUE;
	if(Team == -1)
		return false;
		
	CFlag *F = new CFlag(&GameServer()->m_World, Team, Pos, 0x0);
	m_apFlags[Team] = F;
	return true;
}

bool CGameControllerFC::IsOwnFlagStand(vec2 Pos, int Team)
{
	for(int fi = 0; fi < 2; fi++)
	{
		CFlag *F = m_apFlags[fi];
			
		if(F && F->m_Team == Team && distance(F->m_Pos, Pos) < 32)
			return true;
	}
	
	return false;
}

bool CGameControllerFC::IsEnemyFlagStand(vec2 Pos, int Team)
{
	for(int fi = 0; fi < 2; fi++)
	{
		CFlag *F = m_apFlags[fi];
			
		if(F && F->m_Team != Team && distance(F->m_Pos, Pos) < 32)
			return true;
	}
	
	return false;
}

int CGameControllerFC::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	int ID = pVictim->GetPlayer()->GetCID();
	if(m_apPlFlags[ID])
	{
		m_apPlFlags[ID]->Reset();
		m_apPlFlags[ID] = 0;
	}

	return CGameControllerNoTeamRace::OnCharacterDeath(pVictim, pKiller, Weapon);
}

void CGameControllerFC::OnCharacterSpawn(class CCharacter *pChr)
{
	IGameController::OnCharacterSpawn(pChr);
	
	//full armor
	pChr->IncreaseArmor(10);
	
	// give nades
	if(!g_Config.m_SvNoItems)
		pChr->GiveWeapon(WEAPON_GRENADE, 10);
}

bool CGameControllerFC::CanSpawn(CPlayer *pPlayer, vec2 *pOutPos)
{
	CSpawnEval Eval;
	
	// spectators can't spawn
	if(pPlayer->GetTeam() == -1)
		return false;

	Eval.m_FriendlyTeam = pPlayer->GetTeam();
	
	// try first enemy spawns, than normal, than own
	EvaluateSpawnType(&Eval, 1+((pPlayer->GetTeam()+1)&1));
	if(!Eval.m_Got)
	{
		EvaluateSpawnType(&Eval, 0);
		if(!Eval.m_Got)
			EvaluateSpawnType(&Eval, 1+(pPlayer->GetTeam()&1));
	}
	
	*pOutPos = Eval.m_Pos;
	return Eval.m_Got;
}

bool CGameControllerFC::CanBeMovedOnBalance(int ClientID)
{
	CCharacter* Character = GameServer()->m_apPlayers[ClientID]->GetCharacter();
	if(Character)
	{
		for(int fi = 0; fi < 2; fi++)
		{
			CFlag *F = m_apFlags[fi];
			if(F->m_pCarryingCharacter == Character)
				return false;
		}
	}
	return true;
}

bool CGameControllerFC::OnRaceStart(int ClientID, float StartAddTime, bool Check)
{
	CRaceData *p = &m_aRace[ClientID];
	if(p->m_RaceState == RACE_STARTED)
		return false;
	
	CGameControllerNoTeamRace::OnRaceStart(ClientID, StartAddTime, false);
	
	m_apPlFlags[ClientID] = new CFlag(&GameServer()->m_World, GameServer()->m_apPlayers[ClientID]->GetTeam()^1, GameServer()->GetPlayerChar(ClientID)->m_Pos, GameServer()->GetPlayerChar(ClientID));
	GameServer()->CreateSoundGlobal(SOUND_CTF_GRAB_EN, ClientID);

	return true;
}

bool CGameControllerFC::OnRaceEnd(int ClientID, float FinishTime)
{
	if(!CGameControllerNoTeamRace::OnRaceEnd(ClientID, FinishTime))
		return false;

	if(m_apPlFlags[ClientID])
	{
		m_apPlFlags[ClientID]->Reset();
		m_apPlFlags[ClientID] = 0;

		// respawn pickups
		for(CEntity *pEnt = GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_PICKUP); pEnt; pEnt = pEnt->TypeNext())
			((CPickup *)pEnt)->Respawn(ClientID);

		// sound \o/
		GameServer()->CreateSoundGlobal(SOUND_CTF_CAPTURE, ClientID);
	}

	return true;
}

void CGameControllerFC::Snap(int SnappingClient)
{
	IGameController::Snap(SnappingClient);

	CNetObj_GameData *pGameDataObj = (CNetObj_GameData *)Server()->SnapNewItem(NETOBJTYPE_GAMEDATA, 0, sizeof(CNetObj_GameData));
	if(!pGameDataObj)
		return;

	pGameDataObj->m_TeamscoreRed = 0;
	pGameDataObj->m_TeamscoreBlue = 0;

	if(m_apPlFlags[SnappingClient])
	{
		if(m_apPlFlags[SnappingClient]->m_Team == TEAM_RED)
		{
			pGameDataObj->m_FlagCarrierRed = SnappingClient;
			pGameDataObj->m_FlagCarrierBlue = FLAG_MISSING;
		}
		else if(m_apPlFlags[SnappingClient]->m_Team == TEAM_BLUE)
		{
			pGameDataObj->m_FlagCarrierBlue = SnappingClient;
			pGameDataObj->m_FlagCarrierRed = FLAG_MISSING;
		}
	}
	else
	{
		pGameDataObj->m_FlagCarrierRed = FLAG_MISSING;
		pGameDataObj->m_FlagCarrierBlue = FLAG_MISSING;
	}
		
}
