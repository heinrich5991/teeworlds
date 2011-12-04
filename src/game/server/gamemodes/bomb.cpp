/* (c) heinrich5991 */

#include "bomb.h"

#include <game/mapitems.h>

#include <game/server/gamecontext.h>

CGameControllerBOMB::CGameControllerBOMB(CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "BOMB";
	m_Bomb.m_ClientID = -1;
	m_BombEndTick = -1;
	for(int i = 0; i < MAX_CLIENTS; i++)
		m_aClients[i].m_State = STATE_ACTIVE;
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
				GameServer()->SendChatTarget(i, aBuf);
			else
			{
				GameServer()->SendChatTarget(i, "You are the new bomb!");
				GameServer()->SendChatTarget(i, "Hit another player before the time runs out!");
			}
		}
	}
}

void CGameControllerBOMB::StartBombRound()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
		if(GameServer()->m_apPlayers[i] && m_aClients[i].m_State == STATE_ACTIVE)
		{
			GameServer()->m_apPlayers[i]->SetTeam(TEAM_RED, true);
			GameServer()->m_apPlayers[i]->Respawn();
			m_aClients[i].m_State = STATE_ALIVE;
		}

	m_Bomb.m_ClientID = -1;
	m_Bomb.m_Tick = SERVER_TICK_SPEED * 19 + 1;
}

void CGameControllerBOMB::EndBombRound()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
		if(GameServer()->m_apPlayers[i] && m_aClients[i].m_State >= STATE_ALIVE)
			GameServer()->m_apPlayers[i]->m_Score++;

	EndRound();
}

void CGameControllerBOMB::MakeRandomBomb()
{
	m_Bomb.m_Tick = SERVER_TICK_SPEED * 19 + 1;

	int Alive[MAX_CLIENTS];
	int NumAlives = 0;

	for(int i = 0; i < MAX_CLIENTS; i++)
		if(GameServer()->m_apPlayers[i] && m_aClients[i].m_State >= STATE_ALIVE)
			Alive[NumAlives++] = i;

	if(NumAlives)
		MakeBomb(Alive[rand() % NumAlives]);
}

void CGameControllerBOMB::DoWincheck()
{
	if(m_GameOverTick == -1 && !m_Warmup && !GameServer()->m_World.m_ResetRequested)
	{
		int NumActivePlayers = 0;
		int NumLivingPlayers = 0;
		int NumPlayers = 0;

		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(GameServer()->m_apPlayers[i])
			{
				NumPlayers++;
				if(m_aClients[i].m_State >= STATE_ACTIVE)
				{
					NumActivePlayers++;
					if(m_aClients[i].m_State >= STATE_ALIVE)
						NumLivingPlayers++;
					else if(GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
						GameServer()->m_apPlayers[i]->SetTeam(TEAM_SPECTATORS, true);
				}
			}
		}

		if(NumActivePlayers >= 1)
		{
			if(NumLivingPlayers <= 1 && NumActivePlayers > 1)
			{
				EndBombRound();
				GameServer()->SendBroadcast("Round is over!", -1);
				GameServer()->CreateSoundGlobal(SOUND_CTF_CAPTURE);
			}
		}
		else
			EndBombRound();
	}
}

void CGameControllerBOMB::PostReset()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
		if(GameServer()->m_apPlayers[i])
			GameServer()->m_apPlayers[i]->Respawn();
}

bool CGameControllerBOMB::CanJoinTeam(int Team, int NotThisID, char *pBuffer, int BufferSize)
{
	if((GameServer()->m_apPlayers[NotThisID] && GameServer()->m_apPlayers[NotThisID]->GetTeam() == TEAM_SPECTATORS)
		|| Team == TEAM_SPECTATORS)
	{
		if(m_aClients[NotThisID].m_State >= STATE_ACTIVE || Team == TEAM_SPECTATORS)
		{
			m_aClients[NotThisID].m_State = STATE_SPECTATING;
			str_copy(pBuffer, "You are now a spectator\nYou won't join when the round is over", BufferSize);
			return Team == TEAM_SPECTATORS;
		}
		else if (Team != TEAM_SPECTATORS)
		{
			m_aClients[NotThisID].m_State = STATE_ACTIVE;
			str_copy(pBuffer, "You will join the game when the round is over", BufferSize);
			return false;
		}
	}
	return false;
}

void CGameControllerBOMB::OnCharacterSpawn(CCharacter *pChr)
{
	pChr->IncreaseHealth(10);
	pChr->GiveWeapon(WEAPON_HAMMER, -1);
	OnPlayerInfoChange(pChr->GetPlayer());
}

int CGameControllerBOMB::OnCharacterDeath(CCharacter *pChr, CPlayer *pKiller, int Weapon)
{
	if(m_aClients[pChr->GetPlayer()->GetCID()].m_State >= STATE_ACTIVE)
	{
		GameServer()->SendBroadcast("You will automatically rejoin the game when the round is over", pChr->GetPlayer()->GetCID());
		m_aClients[pChr->GetPlayer()->GetCID()].m_State = STATE_ACTIVE;
	}
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

	if(m_GameOverTick == -1)
	{
		if(!m_Warmup && m_Bomb.m_ClientID == -1)
			MakeRandomBomb();
		else if(!m_Warmup)
		{
			if(m_Bomb.m_Tick)
				m_Bomb.m_Tick--;

			if(m_Bomb.m_Tick <= 0)
			{
				GameServer()->SendChat(-1, CGameContext::CHAT_ALL, "BOOM!");
				GameServer()->CreateExplosion(GameServer()->m_apPlayers[m_Bomb.m_ClientID]->m_ViewPos, m_Bomb.m_ClientID, WEAPON_GAME, false);
				GameServer()->CreateSound(GameServer()->m_apPlayers[m_Bomb.m_ClientID]->m_ViewPos, SOUND_GRENADE_EXPLODE);
				GameServer()->m_apPlayers[m_Bomb.m_ClientID]->KillCharacter();
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
	}

	DoWincheck();
	IGameController::Tick();
}

