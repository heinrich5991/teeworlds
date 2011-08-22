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

#include "team_racecontroller.h"

CGameControllerTeamRace::CGameControllerTeamRace(class CGameContext *pGameServer) : IGameControllerRace(pGameServer)
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		m_aRace[i].Reset();
		m_aPlayerRace[i].Reset();
	}
}

int CGameControllerTeamRace::GetAutoGameTeam(int ClientID)
{
	return ClientID;
}

int CGameControllerTeamRace::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
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

void CGameControllerTeamRace::Tick()
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

	for(int i = 0; i < MAX_TEAMS; i++)
	{
		if(Server()->Tick()-m_aRace[i].m_RefreshTime >= Server()->TickSpeed())
			m_aRace[i].m_RefreshTime = Server()->Tick();
	}
		
}

bool CGameControllerTeamRace::OnCheckpoint(int ClientID, int z)
{
	int GameTeam = GameServer()->m_apPlayers[ClientID]->GetGameTeam();
	CRaceData *p = &m_aRace[GameTeam];

	if(p->m_RaceState != RACE_STARTED)
		return false;

	p->m_aCpCurrent[z] = GetTime(GameTeam);

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!GameServer()->m_apPlayers[i] || GameServer()->m_apPlayers[i]->GetGameTeam() != GameTeam)
			continue;

		CPlayerData *pBest = GameServer()->Score()->PlayerData(ClientID);

		if(pBest->m_Time && pBest->m_aCpTime[z] != 0)
		{
			m_aPlayerRace[i].m_CpDiff = p->m_aCpCurrent[z] - pBest->m_aCpTime[z];
			p->m_CpTick = Server()->Tick() + Server()->TickSpeed()*2;
		}
	}

	return true;
}

bool CGameControllerTeamRace::OnRaceStart(int ClientID, float StartAddTime, bool Check)
{
	int GameTeam = GameServer()->m_apPlayers[ClientID]->GetGameTeam();
	CRaceData *p = &m_aRace[GameTeam];

	if(p->m_RaceState != RACE_FINISHED)
		m_aPlayerRace[ClientID].m_State = RACE_STARTED;

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

		m_aPlayerRace[i].m_State = (i == ClientID) ? RACE_STARTED : RACE_TEAM_STARTED;
	}

#if defined(CONF_TEERACE)
	if(Server()->GetUserID(ClientID) > 0 && GameServer()->Webapp()->CurrentMap()->m_ID > -1 && !Server()->IsGhostRecording(ClientID))
		Server()->StartGhostRecord(ClientID, pChr->GetPlayer()->m_TeeInfos.m_SkinName, pChr->GetPlayer()->m_TeeInfos.m_UseCustomColor, pChr->GetPlayer()->m_TeeInfos.m_ColorBody, pChr->GetPlayer()->m_TeeInfos.m_ColorFeet);
#endif

	return true;
}

bool CGameControllerTeamRace::OnRaceEnd(int ClientID, float FinishTime)
{ 
	int GameTeam = GameServer()->m_apPlayers[ClientID]->GetGameTeam();
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
		pParams->m_UserID = Server()->GetUserID(ClientID);
		pParams->m_ClientID = ClientID;
		str_copy(pParams->m_aName, Server()->ClientName(ClientID), MAX_NAME_LENGTH);
		str_copy(pParams->m_aClan, Server()->ClientClan(ClientID), MAX_CLAN_LENGTH);
		pParams->m_Time = FinishTime;
		mem_copy(pParams->m_aCpTime, p->m_aCpCurrent, sizeof(pParams->m_aCpTime));
		
		if(NewRecord && Server()->GetUserID(ClientID) > 0)
		{
			// set demo and ghost so that it is saved
			Server()->SaveGhostDemo(ClientID);
			pParams->m_Tick = Server()->Tick();
		}
		
		if(GameServer()->Webapp()->CurrentMap()->m_ID > -1)
			GameServer()->Webapp()->AddJob(CWebRun::Post, pParams);
		
		// higher run count
		GameServer()->Webapp()->CurrentMap()->m_RunCount++;
	}
	
	// set stop record tick
	if(Server()->IsRecording(ClientID))
		m_aStopRecordTick[ClientID] = Server()->Tick()+Server()->TickSpeed();
	
	// stop ghost record
	if(Server()->IsGhostRecording(ClientID))
		Server()->StopGhostRecord(ClientID, FinishTime);
#endif

	return true;
}

float CGameControllerTeamRace::GetTime(int ClientID)
{
	return (float)(Server()->Tick()-m_aRace[GameServer()->m_apPlayers[ClientID]->GetGameTeam()].m_StartTime)/((float)Server()->TickSpeed());
}

