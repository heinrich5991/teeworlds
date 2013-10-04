/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <base/math.h>
#include <base/vmath.h>

#include <math.h>
#include <engine/map.h>
#include <engine/kernel.h>

#include <game/mapitems.h>
#include <game/layers.h>
#include <game/collision.h>

CCollision::CCollision()
{
	for(int t = 0; t < NUM_GAMELAYERTYPES; t++)
	{
		m_apTiles[t] = 0;
		m_aWidth[t] = 0;
		m_aHeight[t] = 0;
	}
	m_pLayers = 0;
}

void CCollision::Init(class CLayers *pLayers)
{
	m_pLayers = pLayers;
	m_aWidth[GAMELAYERTYPE_VANILLA] = m_pLayers->GameLayer(GAMELAYERTYPE_VANILLA)->m_Width;
	m_aHeight[GAMELAYERTYPE_VANILLA] = m_pLayers->GameLayer(GAMELAYERTYPE_VANILLA)->m_Height;
	m_apTiles[GAMELAYERTYPE_VANILLA] = static_cast<CTile *>(m_pLayers->Map()->GetData(m_pLayers->GameLayer(GAMELAYERTYPE_VANILLA)->m_Data));
	m_aWidth[GAMELAYERTYPE_FREEZE] = m_pLayers->GameLayer(GAMELAYERTYPE_FREEZE)->m_Width;
	m_aHeight[GAMELAYERTYPE_FREEZE] = m_pLayers->GameLayer(GAMELAYERTYPE_FREEZE)->m_Height;
	m_apTiles[GAMELAYERTYPE_FREEZE] = static_cast<CTile *>(m_pLayers->Map()->GetData(m_pLayers->GameLayer(GAMELAYERTYPE_FREEZE)->m_Data));

	for(int i = 0; i < m_aWidth[GAMELAYERTYPE_VANILLA]*m_aHeight[GAMELAYERTYPE_VANILLA]; i++)
	{
		int Index = m_apTiles[GAMELAYERTYPE_VANILLA][i].m_Index;

		if(Index > 128)
			continue;

		switch(Index)
		{
		case TILE_DEATH:
			m_apTiles[GAMELAYERTYPE_FREEZE][i].m_Index = COLFLAG_DEATH;
			break;
		case TILE_SOLID:
			m_apTiles[GAMELAYERTYPE_VANILLA][i].m_Index = COLFLAG_SOLID;
			break;
		case TILE_NOHOOK:
			m_apTiles[GAMELAYERTYPE_VANILLA][i].m_Index = COLFLAG_SOLID|COLFLAG_NOHOOK;
			break;
		default:
			m_apTiles[GAMELAYERTYPE_VANILLA][i].m_Index = 0;
		}
	}

	for(int i = 0; i < m_aWidth[GAMELAYERTYPE_FREEZE]*m_aHeight[GAMELAYERTYPE_FREEZE]; i++)
	{
		int Index = m_apTiles[GAMELAYERTYPE_FREEZE][i].m_Index;

		switch(Index)
		{
		case TILE_FREEZE:
			m_apTiles[GAMELAYERTYPE_FREEZE][i].m_Index = FREEZEFLAG_FREEZE;
			break;
		case TILE_UNFREEZE:
			m_apTiles[GAMELAYERTYPE_FREEZE][i].m_Index = FREEZEFLAG_UNFREEZE;
			break;
		case TILE_DEEP_FREEZE:
			m_apTiles[GAMELAYERTYPE_FREEZE][i].m_Index = FREEZEFLAG_DEEP_FREEZE;
			break;
		case TILE_DEEP_UNFREEZE:
			m_apTiles[GAMELAYERTYPE_FREEZE][i].m_Index = FREEZEFLAG_DEEP_UNFREEZE;
			break;
		case TILE_FREEZE_DEEP_UNFREEZE:
			m_apTiles[GAMELAYERTYPE_FREEZE][i].m_Index = FREEZEFLAG_DEEP_UNFREEZE|FREEZEFLAG_FREEZE;
			break;
		case TILE_UNFREEZE_DEEP_UNFREEZE:
			m_apTiles[GAMELAYERTYPE_FREEZE][i].m_Index = FREEZEFLAG_DEEP_UNFREEZE|FREEZEFLAG_UNFREEZE;
			break;
		default:
			m_apTiles[GAMELAYERTYPE_FREEZE][i].m_Index = 0;
		}
	}
}

int CCollision::GetTile(int x, int y)
{
	ivec2 Pos = GetTilePos(x, y);
	int Index = GetPosIndex(Pos.x, Pos.y, GAMELAYERTYPE_VANILLA);

	return m_apTiles[GAMELAYERTYPE_VANILLA][Index].m_Index > 128 ? 0 : m_apTiles[GAMELAYERTYPE_VANILLA][Index].m_Index;
}

ivec2 CCollision::GetTilePos(float x, float y)
{
	int Nx = clamp(round(x)/32, 0, m_aWidth[GAMELAYERTYPE_VANILLA]-1);
	int Ny = clamp(round(y)/32, 0, m_aHeight[GAMELAYERTYPE_VANILLA]-1);

	return ivec2(Nx, Ny);
}

int CCollision::GetPosIndex(int x, int y, int Layer)
{
	return y*m_aWidth[Layer]+x;
}

bool CCollision::IsTileSolid(int x, int y)
{
	return GetTile(x, y)&COLFLAG_SOLID;
}

// TODO: rewrite this smarter!
int CCollision::IntersectLine(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision)
{
	float Distance = distance(Pos0, Pos1);
	int End(Distance+1);
	vec2 Last = Pos0;

	for(int i = 0; i < End; i++)
	{
		float a = i/Distance;
		vec2 Pos = mix(Pos0, Pos1, a);
		if(CheckPoint(Pos.x, Pos.y))
		{
			if(pOutCollision)
				*pOutCollision = Pos;
			if(pOutBeforeCollision)
				*pOutBeforeCollision = Last;
			return GetCollisionAt(Pos.x, Pos.y);
		}
		Last = Pos;
	}
	if(pOutCollision)
		*pOutCollision = Pos1;
	if(pOutBeforeCollision)
		*pOutBeforeCollision = Pos1;
	return 0;
}

// TODO: OPT: rewrite this smarter!
void CCollision::MovePoint(vec2 *pInoutPos, vec2 *pInoutVel, float Elasticity, int *pBounces)
{
	if(pBounces)
		*pBounces = 0;

	vec2 Pos = *pInoutPos;
	vec2 Vel = *pInoutVel;
	if(CheckPoint(Pos + Vel))
	{
		int Affected = 0;
		if(CheckPoint(Pos.x + Vel.x, Pos.y))
		{
			pInoutVel->x *= -Elasticity;
			if(pBounces)
				(*pBounces)++;
			Affected++;
		}

		if(CheckPoint(Pos.x, Pos.y + Vel.y))
		{
			pInoutVel->y *= -Elasticity;
			if(pBounces)
				(*pBounces)++;
			Affected++;
		}

		if(Affected == 0)
		{
			pInoutVel->x *= -Elasticity;
			pInoutVel->y *= -Elasticity;
		}
	}
	else
	{
		*pInoutPos = Pos + Vel;
	}
}

bool CCollision::TestBox(vec2 Pos, vec2 Size)
{
	Size *= 0.5f;
	if(CheckPoint(Pos.x-Size.x, Pos.y-Size.y))
		return true;
	if(CheckPoint(Pos.x+Size.x, Pos.y-Size.y))
		return true;
	if(CheckPoint(Pos.x-Size.x, Pos.y+Size.y))
		return true;
	if(CheckPoint(Pos.x+Size.x, Pos.y+Size.y))
		return true;
	return false;
}

int CCollision::MoveBox(vec2 *pInoutPos, vec2 *pInoutVel, CTriggers *pOutTriggers, vec2 Size, float Elasticity)
{
	// do the move
	vec2 Pos = *pInoutPos;
	vec2 Vel = *pInoutVel;

	float Distance = length(Vel);
	int Max = (int)Distance;
	int NumTiles = 0;

	if(Distance > 0.00001f)
	{
		//vec2 old_pos = pos;
		float Fraction = 1.0f/(float)(Max+1);
		ivec2 OldPos = GetTilePos(Pos.x, Pos.y);
		bool First = true;
		for(int i = 0; i <= Max; i++)
		{
			//float amount = i/(float)max;
			//if(max == 0)
				//amount = 0;

			vec2 NewPos = Pos + Vel*Fraction; // TODO: this row is not nice

			if(TestBox(vec2(NewPos.x, NewPos.y), Size))
			{
				int Hits = 0;

				if(TestBox(vec2(Pos.x, NewPos.y), Size))
				{
					NewPos.y = Pos.y;
					Vel.y *= -Elasticity;
					Hits++;
				}

				if(TestBox(vec2(NewPos.x, Pos.y), Size))
				{
					NewPos.x = Pos.x;
					Vel.x *= -Elasticity;
					Hits++;
				}

				// neither of the tests got a collision.
				// this is a real _corner case_!
				if(Hits == 0)
				{
					NewPos.y = Pos.y;
					Vel.y *= -Elasticity;
					NewPos.x = Pos.x;
					Vel.x *= -Elasticity;
				}
			}

			Pos = NewPos;
			ivec2 iPos = GetTilePos(Pos.x, Pos.y);
			if(pOutTriggers && (iPos != OldPos || First))
			{
				OldPos = iPos;
				pOutTriggers[NumTiles] = CTriggers();
				HandleTriggerTiles(iPos.x, iPos.y, pOutTriggers + NumTiles);
				NumTiles++;
			}

			First = false;
		}
	}

	*pInoutPos = Pos;
	*pInoutVel = Vel;


	return NumTiles;
}

void CCollision::HandleTriggerTiles(int x, int y, CTriggers *pOutTriggers)
{
	pOutTriggers->m_Freeze = 0;
	int Index = GetPosIndex(x, y, GAMELAYERTYPE_FREEZE);

	if(m_apTiles[GAMELAYERTYPE_FREEZE][Index].m_Index&FREEZEFLAG_FREEZE)
		pOutTriggers->m_Freeze |= TRIGGERFLAG_FREEZE;
	else if(m_apTiles[GAMELAYERTYPE_FREEZE][Index].m_Index&FREEZEFLAG_UNFREEZE)
		pOutTriggers->m_Freeze |= TRIGGERFLAG_UNFREEZE;

	if(m_apTiles[GAMELAYERTYPE_FREEZE][Index].m_Index&FREEZEFLAG_DEEP_FREEZE)
		pOutTriggers->m_Freeze |= TRIGGERFLAG_DEEP_FREEZE;
	else if(m_apTiles[GAMELAYERTYPE_FREEZE][Index].m_Index&FREEZEFLAG_DEEP_UNFREEZE)
		pOutTriggers->m_Freeze |= TRIGGERFLAG_DEEP_UNFREEZE;
}
