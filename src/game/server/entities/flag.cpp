/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/server/gamecontext.h>
#include "flag.h"
#include "../gamemodes/race.h"

CFlag::CFlag(CGameWorld *pGameWorld, int Team, vec2 Pos, CCharacter *pOwner)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_FLAG, Team)
{
	m_Team = Team;
	m_Pos = Pos;
	m_ProximityRadius = ms_PhysSize;
	m_pCarryingCharacter = pOwner;

	GameServer()->m_World.InsertEntity(this);
}

void CFlag::Reset()
{
	if(!m_pCarryingCharacter)
		return;
		
	GameServer()->m_World.DestroyEntity(this);
}

void CFlag::TickDeferedLate()
{
	if(m_pCarryingCharacter)
		m_Pos = m_pCarryingCharacter->m_Pos;
}

void CFlag::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	if((!m_pCarryingCharacter && GameServer()->m_apPlayers[SnappingClient]->GetTeam() != m_Team && GameServer()->m_apPlayers[SnappingClient]->GetCharacter()
		&& GameServer()->RaceController()->m_aRace[SnappingClient].m_RaceState == CGameControllerRACE::RACE_STARTED)
		||(m_pCarryingCharacter && !GameServer()->m_apPlayers[SnappingClient]->m_ShowOthers && SnappingClient != m_pCarryingCharacter->GetPlayer()->GetCID()))
		return;
	
	CNetObj_Flag *pFlag = (CNetObj_Flag *)Server()->SnapNewItem(NETOBJTYPE_FLAG, m_ID, sizeof(CNetObj_Flag));
	if(!pFlag)
		return;

	pFlag->m_X = (int)m_Pos.x;
	pFlag->m_Y = (int)m_Pos.y;
	pFlag->m_Team = m_Team;
}
