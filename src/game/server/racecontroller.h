/* copyright (c) 2007 rajh and gregwar. Score stuff */
#ifndef GAME_SERVER_GAMEMODES_INTERFACE_RACE_H
#define GAME_SERVER_GAMEMODES_INTERFACE_RACE_H

#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>

class IGameControllerRace : public IGameController
{
public:
	enum
	{
		RACE_NONE = 0,
		RACE_STARTED,
		RACE_FINISHED,
	};
	
	IGameControllerRace(class CGameContext *pGameServer);
	virtual ~IGameControllerRace();
	
	vec2 *m_pTeleporter;
	
#if defined(CONF_TEERACE)
	int m_aStopRecordTick[MAX_CLIENTS];
#endif
	
	void InitTeleporter();

	virtual void DoWincheck();
	virtual void Tick() = 0;
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) = 0;

	virtual bool OnCheckpoint(int ClientID, int z) = 0;
	virtual bool OnRaceStart(int ClientID, float StartAddTime, bool Check=true) = 0;
	virtual bool OnRaceEnd(int ClientID, float FinishTime) = 0;

	virtual float GetTime(int ClientID) = 0;
};

#endif
