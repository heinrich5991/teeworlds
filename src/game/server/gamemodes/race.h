
#ifndef GAME_SERVER_GAMEMODES_RACE_H
#define GAME_SERVER_GAMEMODES_RACE_H

#include <game/server/racecontroller.h>

class CGameControllerRACE : public IRaceController
{
public:
	CGameControllerRACE(class CGameContext *pGameContext);
	~CGameControllerRACE();
};

#endif
