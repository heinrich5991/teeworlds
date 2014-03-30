#include "teamscore.h"
#include "gamecontext.h"
#include "player.h"

CTeamsCore::CTeamsCore()
{
	mp_aTeams[0] = (Team*)mem_alloc(sizeof(Team), 1);
	m_TeamCount = 1;
	m_ResetRequested = false;
	
}

void CTeamsCore::Tick()
{
	if(m_ResetRequested == -1) 
		for(int i = 0; i < m_TeamCount; i++) {
			mp_aTeams[i]->m_TeamWorld.m_ResetRequested = true;
		}
	else if(m_ResetRequested > 0 && m_ResetRequested <= m_TeamCount)
		mp_aTeams[m_ResetRequested]->m_TeamWorld.m_ResetRequested = true;
	else
		m_ResetRequested = 0;

	for(int i = 0; i < m_TeamCount; i++)
		mp_aTeams[i]->m_TeamWorld.Tick();
}

void CTeamsCore::SetTeamMode(int Team, int Mode)
{
	mp_aTeams[Team]->m_Mode = Mode;
}

void CTeamsCore::SetTeamState(int Team, int State)
{
	mp_aTeams[Team]->m_State = State;
}

void CTeamsCore::SetGameServer(CGameContext *pGameServer)
{
	m_pGameServer = pGameServer;
	for(int i = 0; i < m_TeamCount; i++)
		mp_aTeams[i]->m_TeamWorld.SetGameServer(pGameServer);
}

void CTeamsCore::Snap(int SnappingClient)
{
	mp_aTeams[m_pGameServer->m_apPlayers[SnappingClient]->GetDDRTeam()]->m_TeamWorld.Snap(SnappingClient);
}

void CTeamsCore::PostSnap()
{
	for(int i = 0; i < m_TeamCount; i++)
		mp_aTeams[i]->m_TeamWorld.PostSnap();
}