/* copyright (c) 2007 rajh and gregwar. Score stuff */
#ifndef GAME_SERVER_NORACETEAMCONTROLLER_H
#define GAME_SERVER_NORACETEAMCONTROLLER_H

#include "gamecontext.h"
#include "racecontroller.h"

class CGameControllerNoTeamRace : public IGameControllerRace
{
public:

	struct CRaceData
	{
		int m_RaceState;
		int m_StartTime;
		int m_RefreshTime;

		float m_aCpCurrent[25];
		int m_CpTick;
		float m_CpDiff;
		
		float m_StartAddTime;

		void Reset()
		{
			m_RaceState = RACE_NONE;
			m_StartTime = -1;
			m_RefreshTime = -1;
			mem_zero(m_aCpCurrent, sizeof(m_aCpCurrent));
			m_CpTick = -1;
			m_CpDiff = 0;
			m_StartAddTime = 0.0f;
		}
	} m_aRace[MAX_CLIENTS];
	
	CGameControllerNoTeamRace(class CGameContext *pGameServer);
	virtual ~CGameControllerNoTeamRace() {}

	virtual void Tick();
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);

	virtual bool OnCheckpoint(int ClientID, int z);
	virtual bool OnRaceStart(int ClientID, float StartAddTime, bool Check=true);
	virtual bool OnRaceEnd(int ClientID, float FinishTime);

	virtual float GetTime(int ClientID);
};

#endif
