/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/server_data.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include "character.h"
#include "pickup.h"

CPickup::CPickup(CGameWorld *pGameWorld, int Type, int SwitchGroup, bool InvertSwitch)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PICKUP, SwitchGroup, InvertSwitch)
{
	m_Type = Type;
	m_ProximityRadius = PickupPhysSize;

	Reset();

	GameWorld()->InsertEntity(this);
}

void CPickup::Reset()
{
	if (g_pData->m_aPickups[m_Type].m_Spawndelay > 0)
		m_SpawnTick = Server()->Tick() + Server()->TickSpeed() * g_pData->m_aPickups[m_Type].m_Spawndelay;
	else
		m_SpawnTick = -1;
}

void CPickup::Tick()
{
	// wait for respawn
	if(m_SpawnTick > 0)
	{
		if(Server()->Tick() > m_SpawnTick)
		{
			// respawn
			m_SpawnTick = -1;

			if(m_Type == PICKUP_GRENADE || m_Type == PICKUP_SHOTGUN || m_Type == PICKUP_LASER)
				GameServer()->CreateSound(m_Pos, SOUND_WEAPON_SPAWN);
		}
		else
			return;
	}

	if(Active())
	{
		// Check if a player intersected us
		CCharacter *pChr;
		if(m_Type == PICKUP_HEALTH || m_Type == PICKUP_ARMOR)
			pChr = GameWorld()->ClosestCharacter(m_Pos, 20.0f, 0);
		else
			pChr = (CCharacter *)GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER);

		for(; pChr; pChr = (CCharacter *)pChr->TypeNext())
	 	{
			if(pChr && pChr->IsAlive() && distance(m_Pos, pChr->m_Pos) < pChr->m_ProximityRadius+20.0f)
			{
				// player picked us up, is someone was hooking us, let them go
				bool Gave = false;
				switch (m_Type)
				{
					case PICKUP_HEALTH:
						if(pChr->IncreaseHealth(1))
						{
							Gave = true;
							GameServer()->CreateSound(m_Pos, SOUND_PICKUP_HEALTH);
						}
						break;

					case PICKUP_ARMOR:
						if(pChr->IncreaseArmor(1))
						{
							Gave = true;
							GameServer()->CreateSound(m_Pos, SOUND_PICKUP_ARMOR);
						}
						break;

					case PICKUP_GRENADE:
						if(pChr->GiveWeapon(WEAPON_GRENADE))
						{
							Gave = true;
							GameServer()->CreateSound(m_Pos, SOUND_PICKUP_GRENADE);
							if(pChr->GetPlayer())
								GameServer()->SendWeaponPickup(pChr->GetPlayer()->GetCID(), WEAPON_GRENADE);
						}
						break;
					case PICKUP_SHOTGUN:
						if(pChr->GiveWeapon(WEAPON_SHOTGUN))
						{
							Gave = true;
							GameServer()->CreateSound(m_Pos, SOUND_PICKUP_SHOTGUN);
							if(pChr->GetPlayer())
								GameServer()->SendWeaponPickup(pChr->GetPlayer()->GetCID(), WEAPON_SHOTGUN);
						}
						break;
					case PICKUP_LASER:
						if(pChr->GiveWeapon(WEAPON_LASER))
						{
							Gave = true;
							GameServer()->CreateSound(m_Pos, SOUND_PICKUP_SHOTGUN);
							if(pChr->GetPlayer())
								GameServer()->SendWeaponPickup(pChr->GetPlayer()->GetCID(), WEAPON_LASER);
						}
						break;

					case PICKUP_NINJA:
						{
							Gave = true;
							// activate ninja on target player
							pChr->GiveNinja();

							// loop through all players, setting their emotes
							CCharacter *pC = static_cast<CCharacter *>(GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER));
							for(; pC; pC = (CCharacter *)pC->TypeNext())
							{
								if (pC != pChr)
									pC->SetEmote(EMOTE_SURPRISE, Server()->Tick() + Server()->TickSpeed());
							}

							pChr->SetEmote(EMOTE_ANGRY, Server()->Tick() + 1200 * Server()->TickSpeed() / 1000);
							break;
						}

					default:
						break;
				};

				if(Gave)
				{
					char aBuf[256];
					str_format(aBuf, sizeof(aBuf), "pickup player='%d:%s' item=%d/%d",
						pChr->GetPlayer()->GetCID(), Server()->ClientName(pChr->GetPlayer()->GetCID()), m_Type);
					GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
					if(g_pData->m_aPickups[m_Type].m_Respawntime) 
						m_SpawnTick = Server()->Tick() + Server()->TickSpeed() * g_pData->m_aPickups[m_Type].m_Respawntime;
				}
			}

			if(m_Type == PICKUP_HEALTH || m_Type == PICKUP_ARMOR)
				break;
		}
	}
}

void CPickup::TickPaused()
{
	if(m_SpawnTick != -1)
		++m_SpawnTick;
}

void CPickup::Snap(int SnappingClient)
{
	if(m_SpawnTick != -1 || NetworkClipped(SnappingClient) || !Active())
		return;

	CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_ID, sizeof(CNetObj_Pickup)));
	if(!pP)
		return;

	pP->m_X = (int)m_Pos.x;
	pP->m_Y = (int)m_Pos.y;
	pP->m_Type = m_Type;
}
