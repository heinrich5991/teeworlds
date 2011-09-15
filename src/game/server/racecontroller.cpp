/* copyright (c) 2007 rajh, race mod stuff */

#include <engine/storage.h>
#include <engine/shared/config.h>

#include <game/server/gameworld.h>
#include <game/server/entities/character.h>
#include <game/server/entities/pickup.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>
#include <game/server/score.h>

#if defined(CONF_TEERACE)
#include <game/server/webapp.h>
#endif

#include "racecontroller.h"

IRaceController::IRaceController(class CGameContext *pGameServer) : IGameController(pGameServer)
{
	m_pTeleporter = 0;
	
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		m_aRace[i].Reset();
		m_aPlayerRace[i].Reset();
#if defined(CONF_TEERACE)
		m_aStopRecordTick[i] = -1;
#endif
	}
}

IRaceController::~IRaceController()
{
	delete[] m_pTeleporter;
}

void IRaceController::InitTeleporter()
{
	int ArraySize = 0;
	if(GameServer()->Collision()->Layers()->TeleLayer())
	{
		for(int i = 0; i < GameServer()->Collision()->Layers()->TeleLayer()->m_Width*GameServer()->Collision()->Layers()->TeleLayer()->m_Height; i++)
		{
			// get the array size
			if(GameServer()->Collision()->m_pTele[i].m_Number > ArraySize)
				ArraySize = GameServer()->Collision()->m_pTele[i].m_Number;
		}
	}
	
	if(!ArraySize)
	{
		m_pTeleporter = 0x0;
		return;
	}
	
	m_pTeleporter = new vec2[ArraySize];
	mem_zero(m_pTeleporter, ArraySize*sizeof(vec2));
	
	// assign the values
	for(int i = 0; i < GameServer()->Collision()->Layers()->TeleLayer()->m_Width*GameServer()->Collision()->Layers()->TeleLayer()->m_Height; i++)
	{
		if(GameServer()->Collision()->m_pTele[i].m_Number > 0 && GameServer()->Collision()->m_pTele[i].m_Type == TILE_TELEOUT)
			m_pTeleporter[GameServer()->Collision()->m_pTele[i].m_Number-1] = vec2(i%GameServer()->Collision()->Layers()->TeleLayer()->m_Width*32+16, i/GameServer()->Collision()->Layers()->TeleLayer()->m_Width*32+16);
	}
}

int IRaceController::GetAutoGameTeam(int ClientID)
{
	if(CanUsePartnerCommands())
		return -1;
	else
		return ClientID;
}

int IRaceController::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	int ClientID = pVictim->GetPlayer()->GetCID();
	int GameTeam = pVictim->GetPlayer()->GetGameTeam();

	if(m_aRace[GameTeam].m_RaceState == RACE_NONE)
		m_aPlayerRace[ClientID].m_State = RACE_NONE;
	else
		m_aPlayerRace[ClientID].m_State = RACE_TEAM_STARTED;

	bool Reset = 1;
	bool AllDead = 1;

	for(int i = 0; i < MAX_CLIENTS; i++)
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetGameTeam() == GameTeam)
		{
			if(GameServer()->m_apPlayers[i]->GetCharacter() &&  GameServer()->m_apPlayers[i]->GetCharacter()->IsAlive())
				AllDead = 0;
			if(m_aPlayerRace[i].m_State != m_aPlayerRace[ClientID].m_State)
				Reset = 0;
			if(!Reset && !AllDead)
				break;
		}

	if(Reset)
	{
		for(int i = 0; i < MAX_CLIENTS; i++)
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetGameTeam() == GameTeam)
				m_aPlayerRace[i].m_State = RACE_NONE;

		m_aRace[GameTeam].m_RaceState = RACE_NONE;
	}

	if(AllDead)
	{
		// remove projectiles if the player is dead to prevent cheating at start
		if(g_Config.m_SvDeleteGrenadesAfterDeath)
		{
			for(CEntity *pEnt = GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_PROJECTILE); pEnt; pEnt = pEnt->TypeNext())
				if(pEnt->Team() == GameTeam)
					pEnt->Reset();

			for(CEntity *pEnt = GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_LASER); pEnt; pEnt = pEnt->TypeNext())
				if(pEnt->Team() == GameTeam)
					pEnt->Reset();
		}

		// respawn pickups
		for(CEntity *pEnt = GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_PICKUP); pEnt; pEnt = pEnt->TypeNext())
			((CPickup *)pEnt)->Respawn(GameTeam);

#if defined(CONF_TEERACE)
		if(Server()->IsRecording(ClientID))
			Server()->StopRecord(ClientID);
	
		if(Server()->IsGhostRecording(ClientID))
			Server()->StopGhostRecord(ClientID);
#endif
	}

	return 0;
}

void IRaceController::DoWincheck()
{
	if(m_GameOverTick == -1 && !m_Warmup)
	{
		if((g_Config.m_SvTimelimit > 0 && (Server()->Tick()-m_RoundStartTick) >= g_Config.m_SvTimelimit*Server()->TickSpeed()*60))
			EndRound();
	}
}

void IRaceController::Tick()
{
	IGameController::Tick();
	DoWincheck();

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!GameServer()->m_apPlayers[i])
			continue;

		CRaceData *p = &m_aRace[GameServer()->m_apPlayers[i]->GetGameTeam()];

		if((p->m_RaceState == RACE_STARTED) && Server()->Tick()-p->m_RefreshTime >= Server()->TickSpeed())
		{
			int IntTime = (int)GetTime(i);

			char aBuftime[128];
			char aTmp[128];

			CNetMsg_Sv_RaceTime Msg;
			Msg.m_Time = IntTime;
			Msg.m_Check = 0;

			str_format(aBuftime, sizeof(aBuftime), "Current Time: %d min %d sec", IntTime/60, IntTime%60);

			if(p->m_CpTick != -1 && p->m_CpTick > Server()->Tick())
			{
				Msg.m_Check = (int)(m_aPlayerRace[i].m_CpDiff*100);
				str_format(aTmp, sizeof(aTmp), "\nCheckpoint | Diff : %+5.2f", m_aPlayerRace[i].m_CpDiff);
				str_append(aBuftime, aTmp, sizeof(aBuftime));
			}

			if(GameServer()->m_apPlayers[i]->m_IsUsingRaceClient)
				Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, i);
			else
				GameServer()->SendBroadcast(aBuftime, i);
		}

#if defined(CONF_TEERACE)
		// stop recording at the finish
		CPlayerData *pBest = GameServer()->Score()->PlayerData(i);
		if(Server()->IsRecording(i))
		{
			if(Server()->Tick() == m_aStopRecordTick[i])
			{
				m_aStopRecordTick[i] = -1;
				Server()->StopRecord(i);
				continue;
			}
			
			if(m_aRace[i].m_RaceState == RACE_STARTED && pBest->m_Time > 0 && pBest->m_Time < GetTime(i))
				Server()->StopRecord(i);
		}
		
		// stop ghost if time is bigger then best time
		if(Server()->IsGhostRecording(i) && m_aRace[i].m_RaceState == RACE_STARTED && pBest->m_Time > 0 && pBest->m_Time < GetTime(i))
			Server()->StopGhostRecord(i);
#endif
	}

	for(int i = 0; i < 16; i++)
	{
		if(Server()->Tick()-m_aRace[i].m_RefreshTime >= Server()->TickSpeed())
			m_aRace[i].m_RefreshTime = Server()->Tick();
	}
		
}

bool IRaceController::OnCheckpoint(int ID, int z)
{
	int GameTeam = GameServer()->m_apPlayers[ID]->GetGameTeam();
	CRaceData *p = &m_aRace[GameTeam];

	if(p->m_RaceState != RACE_STARTED)
		return false;

	p->m_aCpCurrent[z] = GetTime(GameTeam);

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!GameServer()->m_apPlayers[i] || GameServer()->m_apPlayers[i]->GetGameTeam() != GameTeam)
			continue;

		CPlayerData *pBest = GameServer()->Score()->PlayerData(ID);

		if(pBest->m_Time && pBest->m_aCpTime[z] != 0)
		{
			m_aPlayerRace[i].m_CpDiff = p->m_aCpCurrent[z] - pBest->m_aCpTime[z];
			p->m_CpTick = Server()->Tick() + Server()->TickSpeed()*2;
		}
	}

	return true;
}

bool IRaceController::OnRaceStart(int ID, float StartAddTime, bool Check)
{
	int GameTeam = GameServer()->m_apPlayers[ID]->GetGameTeam();
	CRaceData *p = &m_aRace[GameTeam];

	if(p->m_RaceState != RACE_FINISHED)
		m_aPlayerRace[ID].m_State = RACE_STARTED;

	if(p->m_RaceState != RACE_NONE)
		return false;

	p->m_RaceState = RACE_STARTED;
	p->m_StartTime = Server()->Tick();
	p->m_RefreshTime = Server()->Tick();
	p->m_StartAddTime = StartAddTime;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!GameServer()->m_apPlayers[i] || GameServer()->m_apPlayers[i]->GetGameTeam() != GameTeam)
			continue;

		m_aPlayerRace[i].m_State = (i == ID) ? RACE_STARTED : RACE_TEAM_STARTED;
	}

#if defined(CONF_TEERACE)
	if(Server()->GetUserID(ID) > 0 && GameServer()->Webapp()->CurrentMap()->m_ID > -1 && !Server()->IsGhostRecording(ID))
		Server()->StartGhostRecord(ID, pChr->GetPlayer()->m_TeeInfos.m_SkinName, pChr->GetPlayer()->m_TeeInfos.m_UseCustomColor, pChr->GetPlayer()->m_TeeInfos.m_ColorBody, pChr->GetPlayer()->m_TeeInfos.m_ColorFeet);
#endif

	return true;
}

bool IRaceController::OnRaceEnd(int ID, float FinishTime)
{ 
	int GameTeam = GameServer()->m_apPlayers[ID]->GetGameTeam();
	CRaceData *p = &m_aRace[GameTeam];

	if(p->m_RaceState != RACE_STARTED)
		return false;

	p->m_RaceState = RACE_FINISHED;

	for(int i = 0;  i < MAX_CLIENTS; i++)
	{
		if(!GameServer()->m_apPlayers[i] || GameServer()->m_apPlayers[i]->GetGameTeam() != GameTeam)
			continue;

		m_aPlayerRace[i].m_State = RACE_FINISHED;

		CPlayerData *pBest = GameServer()->Score()->PlayerData(i);

		// add the time from the start
		FinishTime += p->m_StartAddTime;
	
		GameServer()->m_apPlayers[i]->m_Score = max(-(int)FinishTime, GameServer()->m_apPlayers[i]->m_Score);

		float Improved = FinishTime - pBest->m_Time;
		bool NewRecord = pBest->Check(FinishTime, p->m_aCpCurrent);

		// save the score
		if(str_comp_num(Server()->ClientName(i), "nameless tee", 12) != 0 && NewRecord)
		{
			GameServer()->Score()->SaveScore(i);
			if(GameServer()->Score()->CheckRecord(i) && g_Config.m_SvShowTimes)
				GameServer()->SendRecord(-1);
		}

		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "%s finished in: %d minute(s) %6.3f second(s)", Server()->ClientName(i), (int)FinishTime / 60, fmod(FinishTime, 60));
		if(!g_Config.m_SvShowTimes)
			GameServer()->SendChatTarget(i, aBuf);
		else
			GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);

		if(Improved < 0)
		{
			str_format(aBuf, sizeof(aBuf), "New record: %6.3f second(s) better", Improved);
			if(!g_Config.m_SvShowTimes)
				GameServer()->SendChatTarget(i, aBuf);
			else
				GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
		}
	}
	
#if defined(CONF_TEERACE)	
	// post to webapp
	if(GameServer()->Webapp())
	{
		CWebRun::CParam *pParams = new CWebRun::CParam();
		pParams->m_UserID = Server()->GetUserID(ID);
		pParams->m_ClientID = ID;
		str_copy(pParams->m_aName, Server()->ClientName(ID), MAX_NAME_LENGTH);
		str_copy(pParams->m_aClan, Server()->ClientClan(ID), MAX_CLAN_LENGTH);
		pParams->m_Time = FinishTime;
		mem_copy(pParams->m_aCpTime, p->m_aCpCurrent, sizeof(pParams->m_aCpTime));
		
		if(NewRecord && Server()->GetUserID(ID) > 0)
		{
			// set demo and ghost so that it is saved
			Server()->SaveGhostDemo(ID);
			pParams->m_Tick = Server()->Tick();
		}
		
		if(GameServer()->Webapp()->CurrentMap()->m_ID > -1)
			GameServer()->Webapp()->AddJob(CWebRun::Post, pParams);
		
		// higher run count
		GameServer()->Webapp()->CurrentMap()->m_RunCount++;
	}
	
	// set stop record tick
	if(Server()->IsRecording(ID))
		m_aStopRecordTick[ID] = Server()->Tick()+Server()->TickSpeed();
	
	// stop ghost record
	if(Server()->IsGhostRecording(ID))
		Server()->StopGhostRecord(ID, FinishTime);
#endif

	return true;
}

bool IRaceController::CanJoinTeam(int Team, int ClientID)
{
	if(!GameServer()->m_apPlayers[ClientID] || Team == TEAM_SPECTATORS)
		return true;

	return GameServer()->m_apPlayers[ClientID]->GetGameTeam() != -1;
}

int IRaceController::GetAutoTeam(int ClientID)
{
	if(CanUsePartnerCommands())
		return TEAM_SPECTATORS;
	else
		return TEAM_RED;
}

int IRaceController::GetEmptyTeam()
{
	int TeamPlayerCount[16] = { 0 };
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!GameServer()->m_apPlayers[i])
			continue;

		int Team = GameServer()->m_apPlayers[i]->GetGameTeam();
		if(Team == -1)
			continue;
		TeamPlayerCount[Team]++;
	}

	for(int i = 0; i < 16; i++)
	{
		if(TeamPlayerCount[i] == 0)
			return i;
	}

	return -1;
}

void IRaceController::TryCreateTeam(int ClientID, int With)
{
	if(!GameServer()->m_apPlayers[ClientID])
		return;

	if(!GameServer()->m_apPlayers[With])
	{
		GameServer()->SendChatTarget(ClientID, "No such player id");
		return;
	}

	if(ClientID == With)
	{
		GameServer()->SendChatTarget(ClientID, "You can't create a team with yourself");
		return;
	}

	if(GameServer()->m_apPlayers[ClientID]->GetGameTeam() != -1)
	{
		GameServer()->SendChatTarget(ClientID, "You already have a partner");
		return;
	}

	if(GameServer()->m_apPlayers[ClientID]->GetGameTeam() != -1)
	{
		GameServer()->SendChatTarget(ClientID, "Your desired partner already has another partner");
		return;
	}

	m_aPartnerWishes[ClientID] = With;
	
	if(m_aPartnerWishes[With] != ClientID)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "'%s' has been asked", Server()->ClientName(With));
		GameServer()->SendChatTarget(ClientID, aBuf);
		str_format(aBuf, sizeof(aBuf), "'%s' asks you to play with him, type '/with %s' to play", Server()->ClientName(ClientID), Server()->ClientName(ClientID));
		GameServer()->SendChatTarget(With, aBuf);
	}
	else
	{
		int Team = GetEmptyTeam();
		GameServer()->m_apPlayers[ClientID]->SetGameTeam(Team);
		GameServer()->m_apPlayers[With]->SetGameTeam(Team);
		m_aPartnerWishes[ClientID] = -1;
		m_aPartnerWishes[With] = -1;
		GameServer()->m_apPlayers[ClientID]->SetTeam(TEAM_RED);
		GameServer()->m_apPlayers[With]->SetTeam(TEAM_RED);
		GameServer()->m_apPlayers[ClientID]->m_ShowOthers = 0;
		GameServer()->m_apPlayers[With]->m_ShowOthers = 0;

		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(m_aPartnerWishes[i] == ClientID || m_aPartnerWishes[i] == With)
			{
				char aBuf[256];
				str_format(aBuf, sizeof(aBuf), "Your desired partner '%s' has a partner now, ask someone else", Server()->ClientName(m_aPartnerWishes[i]));
				GameServer()->SendChatTarget(i, aBuf);
				m_aPartnerWishes[i] = -1;
			}
		}
	}
}

void IRaceController::ChatCommandWith(int ClientID, const char *pName)
{
	int NumMatches = 0;
	int MatchID = -1;
	if(pName)
	{
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(!GameServer()->m_apPlayers[i])
				continue;
			if(i == ClientID)
				continue;
			else if(str_comp(Server()->ClientName(i), pName) == 0)
			{
				NumMatches = 1;
				MatchID = i;
				break;
			}
			else if(str_find_nocase(Server()->ClientName(i), pName))
			{
				NumMatches++;
				MatchID = i;
			}
		}
	}

	if(NumMatches == 1)
		TryCreateTeam(ClientID, MatchID);
	else if(NumMatches == 0)
		GameServer()->SendChatTarget(ClientID, "No matches found");
	else
		GameServer()->SendChatTarget(ClientID, "More than one match found");
}

void IRaceController::ChatCommandLeaveTeam(int ClientID)
{
	LeaveTeam(ClientID);
}

void IRaceController::LeaveTeam(int ClientID, bool Disconnect)
{
	int Team = GameServer()->m_apPlayers[ClientID]->GetGameTeam();
	if(Team != -1)
	{
		m_aRace[Team].Reset();
		m_aPlayerRace[ClientID].Reset();

		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if((!Disconnect || i != ClientID) && GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetGameTeam() == Team)
			{
				m_aPlayerRace[ClientID].Reset();
				if(i != ClientID)
					GameServer()->SendChatTarget(i, "Your partner has left the team");
				GameServer()->m_apPlayers[i]->SetGameTeam(-1);
				GameServer()->m_apPlayers[i]->SetTeam(TEAM_SPECTATORS);
				GameServer()->m_apPlayers[i]->m_ShowOthers = 1;
			}
		}
	}
}

void IRaceController::OnPlayerDisconnect(CPlayer *pPlayer)
{
	int ClientID = pPlayer->GetCID();

	m_aPartnerWishes[ClientID] = -1;
	LeaveTeam(ClientID, true);
}

float IRaceController::GetTime(int ID)
{
	return (float)(Server()->Tick()-m_aRace[GameServer()->m_apPlayers[ID]->GetGameTeam()].m_StartTime)/((float)Server()->TickSpeed());
}

