/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_FLOW_H
#define GAME_CLIENT_COMPONENTS_FLOW_H
#include <base/vmath.h>
#include <game/client/component.h>

class CFlow : public CComponent
{
	struct CCell
	{
		vec2 m_Vel;
	};

	CCell *m_pCells;
	array<unsigned int> m_pCellsList;

	int m_Height;
	int m_Width;
	int m_Spacing;

	int m_Init;
	void MapscreenToGroup(float CenterX, float CenterY, CMapItemGroup *pGroup);
public:
	CFlow();

	void Init();
	vec2 Get(vec2 Pos);
	void Add(vec2 Pos, vec2 Vel, float Size);
	void Add(unsigned int i, vec2 Vel);
	void Update();
	void DbgRender();
};

#endif
