
#include <game/server/gamecontext.h>

#include "hprace.h"

CGameControllerHPRACE::CGameControllerHPRACE(class CGameContext *pGameServer)
	: CGameControllerTeamRace(pGameServer)
{
	m_pGameType = "HpRace";
	for(int i = 0; i < MAX_CLIENTS; i++)
		m_aPartnerWishes[i] = -1;
}

CGameControllerHPRACE::~CGameControllerHPRACE()
{
}

bool CGameControllerHPRACE::CanJoinTeam(int Team, int ClientID)
{
	if(!GameServer()->m_apPlayers[ClientID] || Team == TEAM_SPECTATORS)
		return true;

	return GameServer()->m_apPlayers[ClientID]->GetGameTeam() != -1;
}

int CGameControllerHPRACE::GetAutoTeam(int ClientID)
{
	return TEAM_SPECTATORS;
}

int CGameControllerHPRACE::GetAutoGameTeam(int ClientID)
{
	return -1;
}

int CGameControllerHPRACE::GetEmptyTeam()
{
	int TeamPlayerCount[16] = { 0 };
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!GameServer()->m_apPlayers[i])
			continue;

		int Team = GameServer()->m_apPlayers[i]->GetGameTeam();
		if(Team == -1)
			continue;
		TeamPlayerCount[Team]++;
	}

	for(int i = 0; i < 16; i++)
	{
		if(TeamPlayerCount[i] == 0)
			return i;
	}

	return -1;
}

void CGameControllerHPRACE::TryCreateTeam(int ClientID, int With)
{
	if(!GameServer()->m_apPlayers[ClientID])
		return;

	if(!GameServer()->m_apPlayers[With])
	{
		GameServer()->SendChatTarget(ClientID, "No such player id");
		return;
	}

	if(ClientID == With)
	{
		GameServer()->SendChatTarget(ClientID, "You can't create a team with yourself");
		return;
	}

	if(GameServer()->m_apPlayers[ClientID]->GetGameTeam() != -1)
	{
		GameServer()->SendChatTarget(ClientID, "You already have a partner");
		return;
	}

	if(GameServer()->m_apPlayers[ClientID]->GetGameTeam() != -1)
	{
		GameServer()->SendChatTarget(ClientID, "Your desired partner already has another partner");
		return;
	}

	m_aPartnerWishes[ClientID] = With;
	
	if(m_aPartnerWishes[With] != ClientID)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "'%s' has been asked", Server()->ClientName(With));
		GameServer()->SendChatTarget(ClientID, aBuf);
		str_format(aBuf, sizeof(aBuf), "'%s' asks you to play with him, type '/with %s' to play", Server()->ClientName(ClientID), Server()->ClientName(ClientID));
		GameServer()->SendChatTarget(With, aBuf);
	}
	else
	{
		int Team = GetEmptyTeam();
		GameServer()->m_apPlayers[ClientID]->SetGameTeam(Team);
		GameServer()->m_apPlayers[With]->SetGameTeam(Team);
		m_aPartnerWishes[ClientID] = -1;
		m_aPartnerWishes[With] = -1;
		GameServer()->m_apPlayers[ClientID]->SetTeam(TEAM_RED);
		GameServer()->m_apPlayers[With]->SetTeam(TEAM_RED);

		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(m_aPartnerWishes[i] == ClientID || m_aPartnerWishes[i] == With)
			{
				char aBuf[256];
				str_format(aBuf, sizeof(aBuf), "Your desired partner '%s' has a partner now, ask someone else", Server()->ClientName(m_aPartnerWishes[i]));
				GameServer()->SendChatTarget(i, aBuf);
				m_aPartnerWishes[i] = -1;
			}
		}
	}
}

void CGameControllerHPRACE::ChatCommandWith(int ClientID, const char *pName)
{
	int NumMatches = 0;
	int MatchID = -1;
	if(pName)
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(!GameServer()->m_apPlayers[i])
				continue;
			if(i == ClientID)
				continue;
			else if(str_comp(Server()->ClientName(i), pName) == 0)
			{
				NumMatches = 1;
				MatchID = i;
				break;
			}
			else if(str_find(Server()->ClientName(i), pName))
			{
				NumMatches++;
				if(NumMatches == 1)
					MatchID = i;
				else
					MatchID = -1;
			}
		}

	if(NumMatches == 1)
		TryCreateTeam(ClientID, MatchID);
	else if(NumMatches == 0)
		GameServer()->SendChatTarget(ClientID, "No matches found");
	else
		GameServer()->SendChatTarget(ClientID, "More than one match found");
}

void CGameControllerHPRACE::ChatCommandLeaveTeam(int ClientID)
{
	LeaveTeam(ClientID);
}

void CGameControllerHPRACE::LeaveTeam(int ClientID, bool Disconnect)
{
	int Team = GameServer()->m_apPlayers[ClientID]->GetGameTeam();
	if(Team != -1)
	{
		m_aRace[Team].Reset();
		m_aPlayerRace[ClientID].Reset();

		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if((!Disconnect || i != ClientID) && GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetGameTeam() == Team)
			{
				m_aPlayerRace[ClientID].Reset();
				if(i != ClientID)
					GameServer()->SendChatTarget(i, "Your partner has left the team");
				GameServer()->m_apPlayers[i]->SetGameTeam(-1);
				GameServer()->m_apPlayers[i]->SetTeam(TEAM_SPECTATORS);
			}
		}
	}
}

void CGameControllerHPRACE::OnPlayerDisconnect(CPlayer *pPlayer)
{
	int ClientID = pPlayer->GetCID();

	m_aPartnerWishes[ClientID] = -1;
	LeaveTeam(ClientID, true);
}

