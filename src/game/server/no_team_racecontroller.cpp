/* copyright (c) 2007 rajh, race mod stuff */
#include <engine/storage.h>
#include <engine/shared/config.h>
#include <game/server/entities/character.h>
#include <game/server/entities/pickup.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>
#include <game/server/score.h>
#if defined(CONF_TEERACE)
#include <game/stream.h>
#include <game/server/webapp.h>
#include <engine/external/json/writer.h>
#endif
#include <stdio.h>
#include <string.h>
#include "no_team_racecontroller.h"

CGameControllerNoTeamRace::CGameControllerNoTeamRace(class CGameContext *pGameServer) : IGameControllerRace(pGameServer)
{
	for(int i = 0; i < MAX_CLIENTS; i++)
		m_aRace[i].Reset();
}

int CGameControllerNoTeamRace::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	int ClientID = pVictim->GetPlayer()->GetCID();
	m_aRace[ClientID].Reset();

	// remove projectiles if the player is dead to prevent cheating at start
	if(g_Config.m_SvDeleteGrenadesAfterDeath)
		for(CEntity *pEnt = GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_PROJECTILE); pEnt; pEnt = pEnt->TypeNext())
			if(pEnt->Team() == ClientID)
				pEnt->Reset();

	// respawn pickups
	for(CEntity *pEnt = GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_PICKUP); pEnt; pEnt = pEnt->TypeNext())
		((CPickup *)pEnt)->Respawn(ClientID);

#if defined(CONF_TEERACE)
	if(Server()->IsRecording(ClientID))
		Server()->StopRecord(ClientID);

	if(Server()->IsGhostRecording(ClientID))
		Server()->StopGhostRecord(ClientID);
#endif

	return 0;
}

void CGameControllerNoTeamRace::Tick()
{
	IGameController::Tick();
	DoWincheck();

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		CRaceData *p = &m_aRace[i];

		if(p->m_RaceState == RACE_STARTED && Server()->Tick()-p->m_RefreshTime >= Server()->TickSpeed())
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
				Msg.m_Check = (int)(p->m_CpDiff*100);
				str_format(aTmp, sizeof(aTmp), "\nCheckpoint | Diff : %+5.2f", p->m_CpDiff);
				strcat(aBuftime, aTmp);
			}

			if(GameServer()->m_apPlayers[i]->m_IsUsingRaceClient)
				Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, i);
			else
				GameServer()->SendBroadcast(aBuftime, i);

			p->m_RefreshTime = Server()->Tick();
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
}

bool CGameControllerNoTeamRace::OnCheckpoint(int ClientID, int z)
{
	CRaceData *p = &m_aRace[ClientID];
	CPlayerData *pBest = GameServer()->Score()->PlayerData(ClientID);
	if(p->m_RaceState != RACE_STARTED)
		return false;

	p->m_aCpCurrent[z] = GetTime(ClientID);

	if(pBest->m_Time && pBest->m_aCpTime[z] != 0)
	{
		p->m_CpDiff = p->m_aCpCurrent[z] - pBest->m_aCpTime[z];
		p->m_CpTick = Server()->Tick() + Server()->TickSpeed()*2;
	}

	return true;
}

bool CGameControllerNoTeamRace::OnRaceStart(int ClientID, float StartAddTime, bool Check)
{
	CRaceData *p = &m_aRace[ClientID];
	CCharacter *pChr = GameServer()->GetPlayerChar(ClientID);
	if(Check && (pChr->HasWeapon(WEAPON_GRENADE) || pChr->Armor()) && (p->m_RaceState == RACE_FINISHED || p->m_RaceState == RACE_STARTED))
		return false;
	
	p->m_RaceState = RACE_STARTED;
	p->m_StartTime = Server()->Tick();
	p->m_RefreshTime = Server()->Tick();
	p->m_StartAddTime = StartAddTime;

	if(p->m_RaceState != RACE_NONE)
	{
		// reset pickups
		if(!pChr->HasWeapon(WEAPON_GRENADE))
			for(CEntity *pEnt = GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_PICKUP); pEnt; pEnt = pEnt->TypeNext())
				((CPickup *)pEnt)->Respawn(ClientID);
	}

#if defined(CONF_TEERACE)
	if(Server()->GetUserID(ClientID) > 0 && GameServer()->Webapp()->CurrentMap()->m_ID > -1 && !Server()->IsGhostRecording(ClientID))
		Server()->StartGhostRecord(ClientID, pChr->GetPlayer()->m_TeeInfos.m_SkinName, pChr->GetPlayer()->m_TeeInfos.m_UseCustomColor, pChr->GetPlayer()->m_TeeInfos.m_ColorBody, pChr->GetPlayer()->m_TeeInfos.m_ColorFeet);
#endif

	return true;
}

bool CGameControllerNoTeamRace::OnRaceEnd(int ClientID, float FinishTime)
{
	CRaceData *p = &m_aRace[ClientID];
	CPlayerData *pBest = GameServer()->Score()->PlayerData(ClientID);
	if(p->m_RaceState != RACE_STARTED)
		return false;

	p->m_RaceState = RACE_FINISHED;

	// add the time from the start
	FinishTime += p->m_StartAddTime;
	
	GameServer()->m_apPlayers[ClientID]->m_Score = max(-(int)FinishTime, GameServer()->m_apPlayers[ClientID]->m_Score);

	float Improved = FinishTime - pBest->m_Time;
	bool NewRecord = pBest->Check(FinishTime, p->m_aCpCurrent);

	// save the score
	if(str_comp_num(Server()->ClientName(ClientID), "nameless tee", 12) != 0 && NewRecord)
	{
		GameServer()->Score()->SaveScore(ClientID);
		if(GameServer()->Score()->CheckRecord(ClientID) && g_Config.m_SvShowTimes)
			GameServer()->SendRecord(-1);
	}

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "%s finished in: %d minute(s) %6.3f second(s)", Server()->ClientName(ClientID), (int)FinishTime/60, fmod(FinishTime,60));
	if(!g_Config.m_SvShowTimes)
		GameServer()->SendChatTarget(ClientID, aBuf);
	else
		GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);

	if(Improved < 0)
	{
		str_format(aBuf, sizeof(aBuf), "New record: %6.3f second(s) better", Improved);
		if(!g_Config.m_SvShowTimes)
			GameServer()->SendChatTarget(ClientID, aBuf);
		else
			GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
	}
	
#if defined(CONF_TEERACE)	
	// post to webapp
	if(GameServer()->Webapp())
	{
		CWebRunData *pUserData = new CWebRunData();
		pUserData->m_UserID = Server()->GetUserID(ClientID);
		pUserData->m_ClientID = ClientID;
		pUserData->m_Tick = -1;

		if(NewRecord && Server()->GetUserID(ClientID) > 0)
		{
			// set demo and ghost so that it is saved
			Server()->SaveGhostDemo(ClientID);
			pUserData->m_Tick = Server()->Tick();
		}

		if(GameServer()->Webapp()->CurrentMap()->m_ID > -1)
		{
			Json::Value Run;
			Json::FastWriter Writer;

			char aBuf[1024];
			Run["map_id"] = GameServer()->Webapp()->CurrentMap()->m_ID;
			Run["map_crc"] = GameServer()->Webapp()->CurrentMap()->m_aCrc;
			Run["user_id"] = Server()->GetUserID(ClientID);
			// TODO: take this out after 0.6 release
			str_copy(aBuf, Server()->ClientName(ClientID), MAX_NAME_LENGTH);
			str_sanitize_strong(aBuf);
			Run["nickname"] = aBuf;
			if(Server()->ClientClan(ClientID)[0])
				Run["clan"] = Server()->ClientClan(ClientID);
			str_format(aBuf, sizeof(aBuf), "%.3f", FinishTime);
			Run["time"] = aBuf;
			float *pCpTime = p->m_aCpCurrent;
			str_format(aBuf, sizeof(aBuf), "%.3f;%.3f;%.3f;%.3f;%.3f;%.3f;%.3f;%.3f;%.3f;%.3f;%.3f;%.3f;%.3f;%.3f;%.3f;%.3f;%.3f;%.3f;%.3f;%.3f;%.3f;%.3f;%.3f;%.3f;%.3f",
				pCpTime[0], pCpTime[1], pCpTime[2], pCpTime[3], pCpTime[4], pCpTime[5], pCpTime[6], pCpTime[7], pCpTime[8], pCpTime[9],
				pCpTime[10], pCpTime[11], pCpTime[12], pCpTime[13], pCpTime[14], pCpTime[15], pCpTime[16], pCpTime[17], pCpTime[18], pCpTime[19],
				pCpTime[20], pCpTime[21], pCpTime[22], pCpTime[23], pCpTime[24], pCpTime[25]);
			Run["checkpoints"] = aBuf;

			std::string Json = Writer.write(Run);

			str_format(aBuf, sizeof(aBuf), CServerWebapp::POST, GameServer()->Webapp()->ApiPath(), "runs/new/",
				GameServer()->Webapp()->ServerIP(), GameServer()->Webapp()->ApiKey(), Json.length(), Json.c_str());
			GameServer()->Webapp()->SendRequest(aBuf, WEB_RUN_POST, new CBufferStream(), pUserData);
		}
		
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

float CGameControllerNoTeamRace::GetTime(int ClientID)
{
	return (float)(Server()->Tick()-m_aRace[ClientID].m_StartTime)/((float)Server()->TickSpeed());
}
