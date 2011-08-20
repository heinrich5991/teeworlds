
#ifndef GAME_SERVER_GAMEMODES_RACE_H
#define GAME_SERVER_GAMEMODES_RACE_H

#include <game/server/no_team_racecontroller.h>

class CGameControllerRACE : public CGameControllerNoTeamRace
{
public:
	CGameControllerRACE(class CGameContext *pGameContext);
	~CGameControllerRACE() {}
};

#endif
