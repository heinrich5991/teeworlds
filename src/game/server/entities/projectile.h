/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_PROJECTILE_H
#define GAME_SERVER_ENTITIES_PROJECTILE_H

class CProjectile : public CEntity
{
public:
	CProjectile(CGameWorld *pGameWorld, int Type, int Owner, vec2 Pos, vec2 Dir, int Span,
		int Damage, bool Explosive, float Force, int SoundImpact, int Weapon);

	vec2 GetPos(float Time);
	vec2 GetDir() { return m_Direction; }
	int GetLifespan() { return m_LifeSpan; }
	bool GetExplosive() { return m_Explosive; }
	int GetSoundImpact() { return m_SoundImpact; }
	int GetStartTick() { return m_StartTick; }
	void FillInfo(CNetObj_Projectile *pProj);
	int GetWeapon() { return m_Weapon; }
	int GetOwner() { return m_Owner; }

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);

private:
	vec2 m_Direction;
	int m_LifeSpan;
	int m_Owner;
	int m_Type;
	int m_Damage;
	int m_SoundImpact;
	int m_Weapon;
	float m_Force;
	int m_StartTick;
	bool m_Explosive;
};

#endif
