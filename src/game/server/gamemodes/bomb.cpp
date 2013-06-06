/* (c) heinrich5991 */

#include "bomb.h"

#include <engine/shared/config.h>

#include <game/mapitems.h>
#include <game/server/gamecontext.h>

CGameControllerBOMB::CGameControllerBOMB(CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "BOMB";
	m_Bomb.m_ClientID = -1;
	m_BombEndTick = -1;
	m_Running = false;
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
				GameServer()->SendBroadcast(aBuf, i);
			else
				GameServer()->SendBroadcast("You are the new bomb\nHit another player before the time runs out!", i);
		}
	}
}

void CGameControllerBOMB::StartBombRound()
{
	m_Bomb.m_ClientID = -1;
	m_Bomb.m_Tick = SERVER_TICK_SPEED * 20;
}

void CGameControllerBOMB::EndBombRound(bool RealEnd)
{
	int Topscore = 0;
	int TopscoreCount = 0;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i] && m_aClients[i].m_State >= STATE_ALIVE)
			GameServer()->m_apPlayers[i]->m_Score++;
		if(GameServer()->m_apPlayers[i] && m_aClients[i].m_State == STATE_ACTIVE)
		{
			GameServer()->m_apPlayers[i]->SetTeam(TEAM_RED, true);
			GameServer()->m_apPlayers[i]->Respawn();
			m_aClients[i].m_State = STATE_ALIVE;
			int Score = GameServer()->m_apPlayers[i]->m_Score;
			if(Score > Topscore)
			{
				Topscore = Score;
				TopscoreCount = 1;
			}
			else if(Score == Topscore)
				TopscoreCount++;
		}

	}

	if(TopscoreCount == 1)
	{
		if(g_Config.m_SvScorelimit && Topscore >= g_Config.m_SvScorelimit)
			RealEnd = true;
		else if(m_SuddenDeath)
			RealEnd = true;
	}

	EndRound(RealEnd);
}

void CGameControllerBOMB::MakeRandomBomb()
{
	m_Bomb.m_Tick = SERVER_TICK_SPEED * 20;

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
					else if(GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS && m_Running)
						GameServer()->m_apPlayers[i]->SetTeam(TEAM_SPECTATORS, true);
				}
			}
		}

		if(!m_Running && NumActivePlayers > 1)
		{
			m_Running = true;
			EndBombRound(true);
			return;
		}

		if(!m_SuddenDeath && g_Config.m_SvTimelimit && (Server()->Tick()-m_RoundStartTick) >= g_Config.m_SvTimelimit * Server()->TickSpeed() * 60)
			m_SuddenDeath = true;

		if(NumActivePlayers >= 1)
		{
			if(NumLivingPlayers <= 1 && NumActivePlayers > 1)
			{
				EndBombRound(false);
				GameServer()->CreateSoundGlobal(SOUND_CTF_CAPTURE);
			}
		}
		else if(m_Running)
		{
			m_Running = false;
			EndBombRound(true);
		}
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
	if(!m_Running && Team != TEAM_SPECTATORS)
	{
		m_aClients[NotThisID].m_State = STATE_ACTIVE;
		return true;
	}
	if(Team == TEAM_SPECTATORS)
	{
		m_aClients[NotThisID].m_State = STATE_SPECTATING;
		if(pBuffer)
			str_copy(pBuffer, "You are a spectator now\nYou won't join when a new round begins", BufferSize);
		return true;
	}
	else
	{
		m_aClients[NotThisID].m_State = STATE_ACTIVE;
		if(pBuffer)
			str_copy(pBuffer, "You will join the game when the round is over", BufferSize);
		return false;
	}
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

	if(m_GameOverTick == -1 && m_Running)
	{
		if(!m_Warmup && m_Bomb.m_ClientID == -1)
			MakeRandomBomb();
		else if(!m_Warmup)
		{
			if(m_Bomb.m_Tick)
				m_Bomb.m_Tick--;

			for(int i = 0; i < MAX_CLIENTS; i++)
				if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetCharacter())
					GameServer()->m_apPlayers[i]->GetCharacter()->SetHealth(m_Bomb.m_Tick / SERVER_TICK_SPEED + 1);

			if(m_Bomb.m_Tick <= 0)
			{
				GameServer()->SendBroadcast("BOOM!", -1);
				GameServer()->CreateExplosion(GameServer()->m_apPlayers[m_Bomb.m_ClientID]->m_ViewPos, m_Bomb.m_ClientID, WEAPON_GAME, false);
				GameServer()->CreateSound(GameServer()->m_apPlayers[m_Bomb.m_ClientID]->m_ViewPos, SOUND_GRENADE_EXPLODE);
				GameServer()->m_apPlayers[m_Bomb.m_ClientID]->KillCharacter();
			}
			else if(m_Bomb.m_Tick % SERVER_TICK_SPEED == 0)
			{
				GameServer()->CreateSound(GameServer()->m_apPlayers[m_Bomb.m_ClientID]->m_ViewPos, SOUND_HOOK_NOATTACH);
				if(m_Bomb.m_Tick / SERVER_TICK_SPEED <= 10)
					GameServer()->CreateDamageInd(GameServer()->m_apPlayers[m_Bomb.m_ClientID]->m_ViewPos, 0, m_Bomb.m_Tick / SERVER_TICK_SPEED);
			}
		}
	}

	DoWincheck();
	IGameController::Tick();
}

