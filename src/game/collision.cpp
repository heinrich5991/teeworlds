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
	m_pTiles = 0;
	m_Width = 0;
	m_Height = 0;
	m_pLayers = 0;
}

void CCollision::Init(class CLayers *pLayers)
{
	m_pLayers = pLayers;
	m_Width = m_pLayers->GameLayer()->m_Width;
	m_Height = m_pLayers->GameLayer()->m_Height;
	m_pTiles = static_cast<CTile *>(m_pLayers->Map()->GetData(m_pLayers->GameLayer()->m_Data));
	// TODO this
	m_pHSpeedupTiles = static_cast<CTile *>(m_pLayers->Map()->GetData((reinterpret_cast<CMapItemLayerTilemap *>(m_pLayers->GetLayer(7)))->m_Data));
	m_pVSpeedupTiles = static_cast<CTile *>(m_pLayers->Map()->GetData((reinterpret_cast<CMapItemLayerTilemap *>(m_pLayers->GetLayer(8)))->m_Data));
	// should be replaced by something like this
	//m_pHSpeedupTiles = static_cast<CTeleTile *>(m_pLayers->Map()->GetData(m_pLayers->HSpeedupLayer()->m_Data));
	//m_pVSpeedupTiles = static_cast<CTeleTile *>(m_pLayers->Map()->GetData(m_pLayers->VSpeedupLayer()->m_Data));
	// once we have a speedup layer

	for(int i = 0; i < m_Width*m_Height; i++)
	{
		int Index = m_pTiles[i].m_Index;

		if(Index > 128)
			continue;

		switch(Index)
		{
		case TILE_DEATH:
			m_pTiles[i].m_Index = COLFLAG_DEATH;
			break;
		case TILE_SOLID:
			m_pTiles[i].m_Index = COLFLAG_SOLID;
			break;
		case TILE_NOHOOK:
			m_pTiles[i].m_Index = COLFLAG_SOLID|COLFLAG_NOHOOK;
			break;
		default:
			m_pTiles[i].m_Index = 0;
		}
	}
}

int CCollision::GetTile(int x, int y)
{
	int Nx = clamp(x/32, 0, m_Width-1);
	int Ny = clamp(y/32, 0, m_Height-1);

	return m_pTiles[Ny*m_Width+Nx].m_Index > 128 ? 0 : m_pTiles[Ny*m_Width+Nx].m_Index;
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
	vec2 SpeedupVel = vec2(0.0f, 0.0f);

	float Distance = length(Vel);
	int Max = (int)Distance;
	int NumTiles = 0;

	if(Distance > 0.00001f)
	{
		//vec2 old_pos = pos;
		float Fraction = 1.0f/(float)(Max+1);
		int Nx = clamp(round(Pos.x)/32, 0, m_Width-1);
		int Ny = clamp(round(Pos.y)/32, 0, m_Height-1);
		int OldPosIndex = Ny*m_Width+Nx;
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

			int Nx = clamp(round(Pos.x)/32, 0, m_Width-1);
			int Ny = clamp(round(Pos.y)/32, 0, m_Height-1);
			int PosIndex = Ny*m_Width+Nx;

			// speedups
			bool Speedup = false;
			if(m_pHSpeedupTiles[PosIndex].m_Index > 0)
			{
				float Accel = m_pHSpeedupTiles[PosIndex].m_Index * Fraction;
				Speedup = true;
				if(m_pHSpeedupTiles[PosIndex].m_Flags&SPEEDUPFLAG_FLIP)
					SpeedupVel.x -= Accel;
				else
					SpeedupVel.x += Accel;
			}
			if(m_pVSpeedupTiles[PosIndex].m_Index > 0)
			{
				float Accel = m_pVSpeedupTiles[PosIndex].m_Index * Fraction;
				Speedup = true;
				if(m_pVSpeedupTiles[PosIndex].m_Flags&SPEEDUPFLAG_FLIP)
					SpeedupVel.y -= Accel;
				else
					SpeedupVel.y += Accel;
			}

			if(pOutTriggers && (PosIndex != OldPosIndex || First))
			{
				pOutTriggers[NumTiles] = CTriggers();

				if(Speedup && PosIndex != OldPosIndex)
					pOutTriggers[NumTiles].m_SpeedupFlags |= TRIGGERFLAG_SPEEDUP;
				HandleTriggerTiles(PosIndex, pOutTriggers + NumTiles);
				NumTiles++;

				OldPosIndex = PosIndex;
			}

			First = false;
		}
	}

	*pInoutPos = Pos;
	*pInoutVel = Vel + SpeedupVel;


	return NumTiles;
}

void CCollision::HandleTriggerTiles(int Index, CTriggers *pOutTriggers)
{

}
