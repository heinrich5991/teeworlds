#ifndef GAME_SERVER_TEAMSCORE_H
#define GAME_SERVER_TEAMSCORE_H

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

struct Team {
	CGameWorld m_TeamWorld;
	int m_State;
	int m_Mode;
};

class CTeamsCore
{
public:
	CTeamsCore();
	int GetTeamMode(int Team) const { return m_apTeams[Team]->m_Mode; }
	int GetTeamState(int Team) const { return m_apTeams[Team]->m_State; }
	CGameWorld *GetTeamWorld(int Team) const { return &m_apTeams[Team]->m_TeamWorld;}
	void SetTeamMode(int Team, int Mode);
	void SetTeamState(int Team, int State);
	void SetGameServer(CGameContext *pGameServer);
	void Tick();
	void Snap(int SnappingClient);
	void PostSnap();

	int m_ResetRequested;
	CGameContext *m_pGameServer;
private:
	int m_TeamCount;
	Team *m_apTeams[MAX_CLIENTS];
};
#endif
