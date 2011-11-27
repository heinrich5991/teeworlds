/* (c) heinrich5991 */

#include "bomb.h"

#include <game/mapitems.h>

#include <game/server/gamecontext.h>

CGameControllerBOMB::CGameControllerBOMB(CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "BOMB";
	m_Bomb.m_ClientID = -1;
	m_Bomb.m_Tick = SERVER_TICK_SPEED * 19 + 1;
}

void CGameControllerBOMB::MakeBomb(int ClientID)
{
	m_Bomb.m_ClientID = ClientID;

	char aBuf[64];
	str_format(aBuf, sizeof(aBuf), "%s is the new bomb!", Server()->ClientName(ClientID));

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i])
		{
			OnPlayerInfoChange(GameServer()->m_apPlayers[i]);
			if(ClientID != i)
				GameServer()->SendBroadcast(aBuf, i);
			else
				GameServer()->SendBroadcast("You are the new bomb!\nHit another player before the time runs out!", i);
		}
	}
}

void CGameControllerBOMB::MakeRandomBomb()
{
	m_Bomb.m_Tick = SERVER_TICK_SPEED * 19 + 1;

	int Active[MAX_CLIENTS];
	int NumActives = 0;

	for(int i = 0; i < MAX_CLIENTS; i++)
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
			Active[NumActives++] = i;

	if(NumActives)
		MakeBomb(Active[rand() % NumActives]);
}

void CGameControllerBOMB::DoWincheck()
{
	if(m_GameOverTick == -1 && !m_Warmup && !GameServer()->m_World.m_ResetRequested)
	{
		int NumActivePlayers = 0;
		int NumPlayers = 0;

		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(GameServer()->m_apPlayers[i])
			{
				NumPlayers++;
				if(GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
					NumActivePlayers++;
			}
		}

		if(NumActivePlayers <= 1)
		{
			if(NumPlayers > 1 || (NumPlayers == 1 && NumActivePlayers == 0))
			{
				EndRound();
				DoWarmup(6);
				GameServer()->SendBroadcast("Round is over!", -1);
				GameServer()->CreateSoundGlobal(SOUND_CTF_CAPTURE);
			}
		}
	}
}

void CGameControllerBOMB::PostReset()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
		if(GameServer()->m_apPlayers[i])
			GameServer()->m_apPlayers[i]->Respawn();
}

void CGameControllerBOMB::OnCharacterSpawn(CCharacter *pChr)
{
	pChr->IncreaseHealth(10);
	pChr->GiveWeapon(WEAPON_HAMMER, -1);
	OnPlayerInfoChange(pChr->GetPlayer());
}

int CGameControllerBOMB::OnCharacterDeath(CCharacter *pChr, CPlayer *pKiller, int Weapon)
{
	return 0;
}

bool CGameControllerBOMB::OnEntity(int Index, vec2 Pos)
{
	if(Index == ENTITY_SPAWN || Index == ENTITY_SPAWN_RED || Index == ENTITY_SPAWN_BLUE)
		return IGameController::OnEntity(Index, Pos);
	return false;
}

void CGameControllerBOMB::OnPlayerInfoChange(CPlayer *pPlayer)
{
	if(m_Bomb.m_ClientID == pPlayer->GetCID())
	{
		str_copy(pPlayer->m_TeeInfos.m_SkinName, "bomb", sizeof(pPlayer->m_TeeInfos.m_SkinName));
		pPlayer->m_TeeInfos.m_UseCustomColor = 0;
	}
	else
	{
		str_copy(pPlayer->m_TeeInfos.m_SkinName, "cammostripes", sizeof(pPlayer->m_TeeInfos.m_SkinName));
		pPlayer->m_TeeInfos.m_UseCustomColor = 1;
		pPlayer->m_TeeInfos.m_ColorBody = 16777215;
		pPlayer->m_TeeInfos.m_ColorFeet = 16777215;
	}
}

void CGameControllerBOMB::Tick()
{
	if(m_Bomb.m_ClientID != -1 && (!GameServer()->m_apPlayers[m_Bomb.m_ClientID] || GameServer()->m_apPlayers[m_Bomb.m_ClientID]->GetTeam() == TEAM_SPECTATORS))
		m_Bomb.m_ClientID = -1;

	if(!m_Warmup && m_Bomb.m_ClientID == -1)
		MakeRandomBomb();
	else if(!m_Warmup)
	{
		if(m_Bomb.m_Tick)
			m_Bomb.m_Tick--;

		if(m_Bomb.m_Tick <= 0)
		{
			GameServer()->SendBroadcast("BOOM!", -1);
			GameServer()->CreateExplosion(GameServer()->m_apPlayers[m_Bomb.m_ClientID]->m_ViewPos, m_Bomb.m_ClientID, WEAPON_GAME, false);
			GameServer()->CreateSound(GameServer()->m_apPlayers[m_Bomb.m_ClientID]->m_ViewPos, SOUND_GRENADE_EXPLODE);
			GameServer()->m_apPlayers[m_Bomb.m_ClientID]->KillCharacter();
			GameServer()->m_apPlayers[m_Bomb.m_ClientID]->m_Score -= 5;

			MakeRandomBomb();
		}
		else if(m_Bomb.m_Tick % SERVER_TICK_SPEED == 0)
		{
			char aBuf[8];
			str_format(aBuf, sizeof(aBuf), "%d", m_Bomb.m_Tick / SERVER_TICK_SPEED);
			GameServer()->SendBroadcast(aBuf, -1);
			GameServer()->CreateSound(GameServer()->m_apPlayers[m_Bomb.m_ClientID]->m_ViewPos, SOUND_HOOK_NOATTACH);
			if(m_Bomb.m_Tick / SERVER_TICK_SPEED <= 10)
				GameServer()->CreateDamageInd(GameServer()->m_apPlayers[m_Bomb.m_ClientID]->m_ViewPos, 0, m_Bomb.m_Tick / SERVER_TICK_SPEED);
		}
	}

	DoWincheck();
	IGameController::Tick();
}
