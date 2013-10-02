/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_COLLISION_H
#define GAME_COLLISION_H

#include <base/vmath.h>
#include <game/mapitems.h>

class CCollision
{
	class CTile *m_apTiles[NUM_GAMELAYERTYPES];
	int m_aWidth[NUM_GAMELAYERTYPES];
	int m_aHeight[NUM_GAMELAYERTYPES];
	class CLayers *m_pLayers;

	vec2 m_aTeleTargets[255];

	bool IsTileSolid(int x, int y);
	int GetTile(int x, int y);
	int GetPosIndex(int x, int y, int Layer);

public:
	enum
	{
		COLFLAG_SOLID=1,
		COLFLAG_DEATH=2,
		COLFLAG_NOHOOK=4,

		TRIGGERFLAG_TELEPORT=1,
		TRIGGERFLAG_CUT_OTHER=2,
		TRIGGERFLAG_CUT_OWN=4,
		TRIGGERFLAG_STOP_NINJA=8,
	};

	struct CTriggers
	{
		int m_TeleFlags;

		vec2 m_TeleInPos;
		vec2 m_TeleOutPos;
		CTriggers() : m_TeleFlags(), m_TeleInPos(vec2(0.0f, 0.0f)), m_TeleOutPos(vec2(0.0f, 0.0f)) {}
	};

	CCollision();
	void Init(class CLayers *pLayers);
	bool CheckPoint(float x, float y) { return IsTileSolid(round(x), round(y)); }
	bool CheckPoint(vec2 Pos) { return CheckPoint(Pos.x, Pos.y); }
	int GetCollisionAt(float x, float y) { return GetTile(round(x), round(y)); }
	int GetWidth() { return m_aWidth[GAMELAYERTYPE_VANILLA]; };
	int GetHeight() { return m_aHeight[GAMELAYERTYPE_VANILLA]; };
	int IntersectLine(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision);
	void MovePoint(vec2 *pInoutPos, vec2 *pInoutVel, float Elasticity, int *pBounces);
	int MoveBox(vec2 *pInoutPos, vec2 *pInoutVel, CTriggers *pOutTriggers, vec2 Size, float Elasticity);
	void HandleTriggerTiles(int x, int y, CTriggers *pOutTriggers);
	bool TestBox(vec2 Pos, vec2 Size);
};

#endif
