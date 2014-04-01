/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/server/gamecontext.h>

#include "character.h"
#include "projectile.h"

CProjectile::CProjectile(CGameWorld *pGameWorld, int Type, int Owner, vec2 Pos, vec2 Dir, int Span,
		int Damage, bool Explosive, float Force, int SoundImpact, int Weapon, int SwitchGroup, bool InvertSwitch, bool OnlySelf)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PROJECTILE, SwitchGroup, InvertSwitch)
{
	m_Type = Type;
	m_Pos = Pos;
	m_Direction = Dir;
	m_StartPos = Pos;
	m_StartDir = Dir;
	m_LifeSpan = Span;
	m_Owner = Owner;
	m_Force = Force;
	m_Damage = Damage;
	m_SoundImpact = SoundImpact;
	m_Weapon = Weapon;
	m_StartTick = Server()->Tick();
	m_Explosive = Explosive;
	m_OnlySelf = OnlySelf;

	GameWorld()->InsertEntity(this);
}

void CProjectile::Reset()
{
	if(m_Type == WEAPON_SHOTGUN)
	{
		m_Pos = m_StartPos;
		m_Direction = m_StartDir;
		m_StartTick = Server()->Tick();
	}	
	else
		GameWorld()->DestroyEntity(this);
}

vec2 CProjectile::GetPos(float Time)
{
	float Curvature = 0;
	float Speed = 0;

	switch(m_Type)
	{
		case WEAPON_GRENADE:
			Curvature = GameServer()->Tuning()->m_GrenadeCurvature;
			Speed = GameServer()->Tuning()->m_GrenadeSpeed;
			break;

		case WEAPON_SHOTGUN:
			Curvature = GameServer()->Tuning()->m_ShotgunCurvature;
			Speed = GameServer()->Tuning()->m_ShotgunSpeed;
			break;

		case WEAPON_GUN:
			Curvature = GameServer()->Tuning()->m_GunCurvature;
			Speed = GameServer()->Tuning()->m_GunSpeed;
			break;
	}

	return CalcPos(m_Pos, m_Direction, Curvature, Speed, Time);
}


void CProjectile::Tick()
{
	float Pt = (Server()->Tick()-m_StartTick-1)/(float)Server()->TickSpeed();
	float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();
	vec2 PrevPos = GetPos(Pt);
	vec2 CurPos = GetPos(Ct);
	vec2 ColPos;
	int Collide = GameServer()->Collision()->IntersectLine(PrevPos, CurPos, &ColPos, 0, CCollision::COLFLAG_SOLID_PROJ);

	if(m_Type == WEAPON_SHOTGUN)
	{
		if(Collide)
		{
			vec2 Vel = CurPos - PrevPos;
			if(GameServer()->Collision()->IntersectLine(PrevPos, vec2(ColPos.x, PrevPos.y), 0, 0, CCollision::COLFLAG_SOLID_PROJ))
				Vel.x *= -1;
			if(GameServer()->Collision()->IntersectLine(PrevPos, vec2(PrevPos.x, ColPos.y), 0, 0, CCollision::COLFLAG_SOLID_PROJ))
				Vel.y *= -1;
			m_Pos = ColPos;
			m_Direction = normalize(Vel);
			m_StartTick = Server()->Tick();
		}

		CCharacter *pChr = (CCharacter *)GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER);
		if(Active()){
			for(; pChr; pChr = (CCharacter *)pChr->TypeNext())
		 	{
				if(pChr && pChr->IsAlive() && distance(CurPos, pChr->m_Pos) < pChr->m_ProximityRadius)
					pChr->Freeze();
			}
		}
	}
	else
	{
		CurPos = ColPos;
		CCharacter *pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
		CCharacter *pTargetChr;
		if(m_OnlySelf)
			pTargetChr = 0;
		else
			pTargetChr = GameWorld()->IntersectCharacter(PrevPos, CurPos, 6.0f, CurPos, pOwnerChar);

		m_LifeSpan--;

		if(pTargetChr || Collide || m_LifeSpan < 0 || GameLayerClipped(CurPos))
		{
			if(m_LifeSpan >= 0 || m_Weapon == WEAPON_GRENADE)
				GameServer()->CreateSound(CurPos, m_SoundImpact);

			if(m_Explosive)
				GameServer()->CreateExplosion(CurPos, m_Owner, m_Weapon, true, m_OnlySelf);

			else if(pTargetChr)
				pTargetChr->TakeDamage(m_Direction * max(0.001f, m_Force), m_Damage, m_Owner, m_Weapon);

			GameWorld()->DestroyEntity(this);
		}
	}
}

void CProjectile::TickPaused()
{
	++m_StartTick;
}

void CProjectile::FillInfo(CNetObj_Projectile *pProj)
{
	pProj->m_X = (int)m_Pos.x;
	pProj->m_Y = (int)m_Pos.y;
	pProj->m_VelX = (int)(m_Direction.x*100.0f);
	pProj->m_VelY = (int)(m_Direction.y*100.0f);
	pProj->m_StartTick = m_StartTick;
	pProj->m_Type = m_Type;
}

void CProjectile::Snap(int SnappingClient)
{
	float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();

	if(NetworkClipped(SnappingClient, GetPos(Ct)))
		return;

	CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_ID, sizeof(CNetObj_Projectile)));
	if(pProj)
		FillInfo(pProj);
	pProj->m_LocalWorld = GameServer()->GetPlayerDDRTeam(SnappingClient) == GameWorld()->DDRTeam();
}
