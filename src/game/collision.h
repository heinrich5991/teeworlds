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
	int m_pOpen;
	class CLayers *m_pLayers;
	bool *m_pSwitchStates;

	vec2 m_aTeleTargets[255];

	int m_NumCheckpoints;

	bool IsTileSolid(int x, int y) { return IsTileSolid(x, y, 0); }
	bool IsTileSolid(int x, int y, int OpenFlags);
	int GetSwitchGroup(int PosIndex, int Layer);
	int GetTile(int x, int y);
	int GetTileFlags(int x, int y);
	ivec2 GetTilePos(float x, float y);
	int GetPosIndex(int x, int y, int Layer);

public:
	enum
	{
		COLFLAG_SOLID=1,
		COLFLAG_DEATH=2,
		COLFLAG_NOHOOK=4,
		COLFLAG_SOLID_HOOK=8,
		COLFLAG_SOLID_PROJ=16,

		DIRFLAG_RIGHT=1,
		DIRFLAG_LEFT=2,
		DIRFLAG_UP=4,
		DIRFLAG_DOWN=8,

		FREEZEFLAG_FREEZE=1,
		FREEZEFLAG_UNFREEZE=2,
		FREEZEFLAG_DEEP_FREEZE=4,
		FREEZEFLAG_DEEP_UNFREEZE=8,

		TRIGGERFLAG_FREEZE=1,
		TRIGGERFLAG_UNFREEZE=2,
		TRIGGERFLAG_DEEP_FREEZE=4,
		TRIGGERFLAG_DEEP_UNFREEZE=8,

		TRIGGERFLAG_SWITCH=1,

		TRIGGERFLAG_TELEPORT=1,
		TRIGGERFLAG_CUT_OTHER=2,
		TRIGGERFLAG_CUT_OWN=4,
		TRIGGERFLAG_STOP_NINJA=8,

		TRIGGERFLAG_SPEEDUP=1,
	};

	struct CTriggers
	{
		int m_FreezeFlags;

		int m_SwitchFlags;
		bool m_SwitchState;
		int m_SwitchGroup;
		int m_SwitchDuration;

		int m_TeleFlags;
		vec2 m_TeleInPos;

		vec2 m_TeleOutPos;
		int m_SpeedupFlags;

		int m_Checkpoint;

		CTriggers() : m_FreezeFlags(), m_SwitchFlags(), m_SwitchState(), m_SwitchGroup(), m_SwitchDuration(), m_TeleFlags(), m_TeleInPos(vec2(0.0f, 0.0f)),
			m_TeleOutPos(vec2(0.0f, 0.0f)), m_SpeedupFlags(), m_Checkpoint() {}
	};

	CCollision();
	void Init(class CLayers *pLayers, bool *pSwitchStates);
	int GetNumCheckpoints();
	bool CheckPoint(float x, float y, int OpenFlags) { return IsTileSolid(round_to_int(x), round_to_int(y), OpenFlags); }
	bool CheckPoint(float x, float y) { return CheckPoint(x, y, ~0); }
	bool CheckPoint(vec2 Pos) { return CheckPoint(Pos.x, Pos.y); }
	int GetCollisionAt(float x, float y) { return GetTile(round_to_int(x), round_to_int(y)); }
	int GetCollisionAt(vec2 Pos) { return GetCollisionAt(Pos.x, Pos.y); }
	int GetWidth() { return m_aWidth[GAMELAYERTYPE_VANILLA]; };
	int GetHeight() { return m_aHeight[GAMELAYERTYPE_VANILLA]; };
	int IntersectLine(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision, int ColFlag);
	int IntersectLineHook(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision);
	int IntersectLineProj(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision);
	int IntersectLine(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision);
	void MovePoint(vec2 *pInoutPos, vec2 *pInoutVel, float Elasticity, int *pBounces);
	void MovePointProj(vec2 *pInoutPos, vec2 *pInoutVel, float Elasticity, int *pBounces);
	void MovePoint(vec2 *pInoutPos, vec2 *pInoutVel, float Elasticity, int *pBounces, int ColFlag);
	int MoveBox(vec2 *pInoutPos, vec2 *pInoutVel, CTriggers *pOutTriggers, vec2 Size, float Elasticity);
	void HandleTriggerTiles(int x, int y, CTriggers *pOutTriggers);
	bool TestBox(vec2 Pos, vec2 Size) { return TestBox(Pos, Size, ~0); }
	bool TestBox(vec2 Pos, vec2 Size, int OpenFlags);
};

#endif
