/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_COLLISION_H
#define GAME_COLLISION_H

#include <base/vmath.h>

class CCollision
{
	class CTile *m_pTiles;
	class CTile *m_pSwitchTiles;
	int m_Width;
	int m_Height;
	class CLayers *m_pLayers;
	bool *m_pSwitchStates;

	bool IsTileSolid(int x, int y);
	int GetPosIndex(int x, int y);
	int GetTile(int PosIndex);
	int GetSwitchGroup(int PosIndex);

public:
	enum
	{
		COLFLAG_SOLID=1,
		COLFLAG_DEATH=2,
		COLFLAG_NOHOOK=4,

		TRIGGERFLAG_SWITCH=1,
	};

	struct CTriggers
	{
		int m_Flags;
		bool m_SwitchState;
		int m_SwitchGroup;
		int m_SwitchDuration;
		CTriggers() : m_Flags(), m_SwitchState(), m_SwitchGroup(), m_SwitchDuration() {}
	};

	CCollision();
	void Init(class CLayers *pLayers, bool *pSwitchStates);
	bool CheckPoint(float x, float y) { return IsTileSolid(round(x), round(y)); }
	bool CheckPoint(vec2 Pos) { return CheckPoint(Pos.x, Pos.y); }
	int GetCollisionAt(float x, float y) { return GetTile(GetPosIndex(round(x), round(y))); }
	int GetWidth() { return m_Width; };
	int GetHeight() { return m_Height; };
	int IntersectLine(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision);
	void MovePoint(vec2 *pInoutPos, vec2 *pInoutVel, float Elasticity, int *pBounces);
	int MoveBox(vec2 *pInoutPos, vec2 *pInoutVel, CTriggers *pOutTriggers, vec2 Size, float Elasticity);
	void HandleTriggerTiles(int Index, CTriggers *pOutTriggers);
	bool TestBox(vec2 Pos, vec2 Size);
};

#endif
