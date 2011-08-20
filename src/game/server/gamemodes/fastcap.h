#ifndef GAME_SERVER_GAMEMODES_CTF_H
#define GAME_SERVER_GAMEMODES_CTF_H

#include "../no_team_racecontroller.h"

class CGameControllerFC : public CGameControllerNoTeamRace
{
public:
	class CFlag *m_apFlags[2];
	class CFlag *m_apPlFlags[MAX_CLIENTS];
	
	CGameControllerFC(class CGameContext *pGameServer);
	
	bool IsOwnFlagStand(vec2 Pos, int Team);
	bool IsEnemyFlagStand(vec2 Pos, int Team);
	
	virtual bool CanBeMovedOnBalance(int ClientID);
	
	virtual bool OnEntity(int Index, vec2 Pos);
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);
	virtual void OnCharacterSpawn(class CCharacter *pChr);
	virtual bool CanSpawn(class CPlayer *pPlayer, vec2 *pOutPos);
	
	virtual bool IsFastCap() { return true; }

	virtual bool OnRaceStart(int ClientID, float StartAddTime, bool Check);
	virtual bool OnRaceEnd(int ClientID, float FinishTime);
	
	virtual void Snap(int SnappingClient);
};

#endif
