
#include "race.h"
#include <game/server/gamecontext.h>

CGameControllerRACE::CGameControllerRACE(CGameContext *pGameContext)
	: CGameControllerNoTeamRace(pGameContext)
{
	m_pGameType = "Race";
}
