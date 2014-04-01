#ifndef GAME_SERVER_TEAMSCORE_H
#define GAME_SERVER_TEAMSCORE_H

#include "eventhandler.h"
#include "gameworld.h"

enum
{
	TEAMSTATE_OPEN = 0,
	TEAMSTATE_LOCKED,
	TEAMSTATE_FULL,
	TEAMSTATE_STARTED,
	TEAMSTATE_FINISHED,
	TEAMMODE_OPEN,
	TEAMMODE_PRIVATE
};

struct CTeam
{
	CGameWorld m_TeamWorld;
	CCollision m_Collision;
	CEventHandler m_Events;
	int m_State;
	int m_Mode;
};

class CTeamsCore
{
public:
	CTeamsCore();
	void InitCollision(class CLayers *pLayers);
	int GetTeamMode(int Team) const { return m_aTeams[Team].m_Mode; }
	int GetTeamState(int Team) const { return m_aTeams[Team].m_State; }
	CGameWorld *GetTeamWorld(int Team) { return &m_aTeams[Team].m_TeamWorld; }
	CCollision *GetTeamCollision(int Team) { return &m_aTeams[Team].m_Collision; }
	CEventHandler *GetTeamEvents(int Team) { return &m_aTeams[Team].m_Events; }
	void SetTeamMode(int Team, int Mode);
	void SetTeamState(int Team, int State);
	void SetGameServer(CGameContext *pGameServer);
	void Tick();
	void Snap(int SnappingClient);
	void PostSnap();
	void Reset(int Team);

	int GetTeamWorldID(CGameWorld *pWorld);

	CGameContext *m_pGameServer;
private:
	CTeam m_aTeams[MAX_CLIENTS];
};
#endif
