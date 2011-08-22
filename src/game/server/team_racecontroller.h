/* copyright (c) 2007 rajh and gregwar. Score stuff */
#ifndef GAME_SERVER_RACETEAMCONTROLLER_H
#define GAME_SERVER_RACETEAMCONTROLLER_H

#include "racecontroller.h"

class CGameControllerTeamRace : public IGameControllerRace
{
public:
	enum
	{
		RACE_TEAM_STARTED = 3,
	};

	struct CRaceData
	{
		int m_RaceState;
		int m_StartTime;
		int m_RefreshTime;

		float m_aCpCurrent[25];
		int m_CpTick;

		float m_StartAddTime;

		void Reset()
		{
			m_RaceState = RACE_NONE;
			m_StartTime = -1;
			m_RefreshTime = -1;
			for(unsigned i = 0; i < sizeof(m_aCpCurrent) / sizeof(m_aCpCurrent[0]); i++)
				m_aCpCurrent[i] = 0.0f;
			m_CpTick = -1;
			m_StartAddTime = 0.0f;
		}
	} m_aRace[MAX_TEAMS];

	struct CPlayerRaceData
	{
		int m_State;
		float m_CpDiff;

		void Reset()
		{
			m_State = RACE_NONE;
			m_CpDiff = 0.0f;
		}
	} m_aPlayerRace[MAX_CLIENTS];

	CGameControllerTeamRace(class CGameContext *pGameServer);
	virtual ~CGameControllerTeamRace() {}

	virtual bool FakeCollisionTune() const { return true; }
	virtual bool FakeHookTune() const { return true; }

	virtual void Tick();
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);

	virtual bool OnCheckpoint(int ClientID, int z);
	virtual bool OnRaceStart(int ClientID, float StartAddTime, bool Check=true);
	virtual bool OnRaceEnd(int ClientID, float FinishTime);

	virtual int GetAutoGameTeam(int ClientID);

	virtual float GetTime(int ClientID);
};

#endif

