#include "teamscore.h"
#include "gamecontext.h"
#include "player.h"

CTeamsCore::CTeamsCore()
{	
}

void CTeamsCore::Tick()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
		m_aTeams[i].m_TeamWorld.Tick();
}

void CTeamsCore::SetTeamMode(int Team, int Mode)
{
	m_aTeams[Team].m_Mode = Mode;
}

void CTeamsCore::SetTeamState(int Team, int State)
{
	m_aTeams[Team].m_State = State;
}

void CTeamsCore::SetGameServer(CGameContext *pGameServer)
{
	m_pGameServer = pGameServer;
	for(int i = 0; i < MAX_CLIENTS; i++)
		m_aTeams[i].m_TeamWorld.SetGameServer(pGameServer);
}

void CTeamsCore::Snap(int SnappingClient)
{
	m_aTeams[m_pGameServer->m_apPlayers[SnappingClient]->GetDDRTeam()].m_TeamWorld.Snap(SnappingClient);
}

void CTeamsCore::PostSnap()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
		m_aTeams[i].m_TeamWorld.PostSnap();
}

void CTeamsCore::Reset(int Team)
{
	if(Team >= 0 && Team < MAX_CLIENTS)
		m_aTeams[Team].m_TeamWorld.m_ResetRequested = true;
	else if(Team == -1) {
		for(int i = 0; i < MAX_CLIENTS; i++)
			m_aTeams[i].m_TeamWorld.m_ResetRequested = true;
	}
}