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
	m_NumCheckpoints = 0;
}

void CCollision::Init(class CLayers *pLayers, bool *pSwitchStates)
{
	m_pSwitchStates = pSwitchStates;
	m_pLayers = pLayers;

	for(int t = 0; t < NUM_GAMELAYERTYPES; t++)
	{
		m_aWidth[t] = m_pLayers->GameLayer(t)->m_Width;
		m_aHeight[t] = m_pLayers->GameLayer(t)->m_Height;
		m_apTiles[t] = static_cast<CTile *>(m_pLayers->Map()->GetData(m_pLayers->GameLayer(t)->m_Data));
	}

	for(int i = 0; i < m_aWidth[GAMELAYERTYPE_VANILLA]*m_aHeight[GAMELAYERTYPE_VANILLA]; i++)
	{
		int Index = m_apTiles[GAMELAYERTYPE_VANILLA][i].m_Index;

		if(Index > 128)
			continue;		

		switch(Index%16)
		{
		case TILE_DEATH:
			m_apTiles[GAMELAYERTYPE_VANILLA][i].m_Index = COLFLAG_DEATH;
			break;
		case TILE_SOLID:
			m_apTiles[GAMELAYERTYPE_VANILLA][i].m_Index = COLFLAG_SOLID|COLFLAG_SOLID_HOOK|COLFLAG_SOLID_PROJ;
			break;
		case TILE_NOHOOK:
			m_apTiles[GAMELAYERTYPE_VANILLA][i].m_Index = COLFLAG_SOLID|COLFLAG_SOLID_HOOK|COLFLAG_SOLID_PROJ|COLFLAG_NOHOOK;
			break;
		case TILE_SEMISOLID_HOOK:
			m_apTiles[GAMELAYERTYPE_VANILLA][i].m_Index = COLFLAG_SOLID|COLFLAG_SOLID_PROJ;
			break;
		case TILE_SEMISOLID_PROJ:
			m_apTiles[GAMELAYERTYPE_VANILLA][i].m_Index = COLFLAG_SOLID|COLFLAG_SOLID_HOOK;
			break;
		case TILE_SEMISOLID_PROJ_NOHOOK:
			m_apTiles[GAMELAYERTYPE_VANILLA][i].m_Index = COLFLAG_SOLID|COLFLAG_SOLID_HOOK|COLFLAG_NOHOOK;
			break;
		case TILE_SEMISOLID_BOTH:
			m_apTiles[GAMELAYERTYPE_VANILLA][i].m_Index = COLFLAG_SOLID;
			break;
		default:
			m_apTiles[GAMELAYERTYPE_VANILLA][i].m_Index = 0;
		}

		if(m_apTiles[GAMELAYERTYPE_VANILLA][i].m_Index&COLFLAG_SOLID)
		{
			int Flags = m_apTiles[GAMELAYERTYPE_VANILLA][i].m_Flags;
			m_apTiles[GAMELAYERTYPE_VANILLA][i].m_Flags = Flags & TILEFLAG_INVERT_SWITCH;

			switch(Index/16)
			{
				case ROW_ONE_OPEN:
					if(Flags&TILEFLAG_ROTATE)
						m_apTiles[GAMELAYERTYPE_VANILLA][i].m_Flags |= Flags&TILEFLAG_HFLIP ? DIRFLAG_LEFT : DIRFLAG_RIGHT;
					else
						m_apTiles[GAMELAYERTYPE_VANILLA][i].m_Flags |= Flags&TILEFLAG_VFLIP ? DIRFLAG_DOWN : DIRFLAG_UP;
					break;
				case ROW_TWO_OPEN:
					if(Flags&TILEFLAG_ROTATE)
						m_apTiles[GAMELAYERTYPE_VANILLA][i].m_Flags |= DIRFLAG_LEFT|DIRFLAG_RIGHT;
					else
						m_apTiles[GAMELAYERTYPE_VANILLA][i].m_Flags |= DIRFLAG_DOWN|DIRFLAG_UP;
					break;
				case ROW_TWO_CORNER_OPEN:
					m_apTiles[GAMELAYERTYPE_VANILLA][i].m_Flags |= Flags&TILEFLAG_HFLIP ? DIRFLAG_LEFT : DIRFLAG_RIGHT;
					if(Flags&TILEFLAG_ROTATE)
						m_apTiles[GAMELAYERTYPE_VANILLA][i].m_Flags |= Flags&TILEFLAG_VFLIP ? DIRFLAG_UP : DIRFLAG_DOWN;
					else
						m_apTiles[GAMELAYERTYPE_VANILLA][i].m_Flags |= Flags&TILEFLAG_VFLIP ? DIRFLAG_DOWN : DIRFLAG_UP;
					break;
				case ROW_THREE_OPEN:
					if(Flags&TILEFLAG_ROTATE)
						m_apTiles[GAMELAYERTYPE_VANILLA][i].m_Flags |= DIRFLAG_UP|DIRFLAG_DOWN|(Flags&TILEFLAG_VFLIP ? DIRFLAG_LEFT : DIRFLAG_RIGHT);
					else
						m_apTiles[GAMELAYERTYPE_VANILLA][i].m_Flags |= DIRFLAG_RIGHT|DIRFLAG_LEFT|(Flags&TILEFLAG_VFLIP ? DIRFLAG_DOWN : DIRFLAG_UP);
					break;
			}
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

	for(int i = 0; i < m_aWidth[GAMELAYERTYPE_TELE]*m_aHeight[GAMELAYERTYPE_TELE]; i++)
		if(m_apTiles[GAMELAYERTYPE_TELE][i].m_Index > 0)
			if(!(m_apTiles[GAMELAYERTYPE_TELE][i].m_Flags&TELEFLAG_IN))
				m_aTeleTargets[m_apTiles[GAMELAYERTYPE_TELE][i].m_Index] = vec2((i%m_aWidth[GAMELAYERTYPE_TELE]+0.5f)*32, (i/m_aWidth[GAMELAYERTYPE_TELE]+0.5f)*32);

	for(int i = 0; i < m_aWidth[GAMELAYERTYPE_RACE]*m_aHeight[GAMELAYERTYPE_RACE]; i++)
		m_NumCheckpoints = max(m_apTiles[GAMELAYERTYPE_RACE][i].m_Index - 2, m_NumCheckpoints);
}

int CCollision::GetNumCheckpoints()
{
	return m_NumCheckpoints;
}

int CCollision::GetSwitchGroup(int PosIndex, int Layer)
{
	return m_apTiles[Layer][PosIndex].m_Reserved - 1;
}

ivec2 CCollision::GetTilePos(float x, float y)
{
	int Nx = clamp(round_to_int(x)/32, 0, m_aWidth[GAMELAYERTYPE_VANILLA]-1);
	int Ny = clamp(round_to_int(y)/32, 0, m_aHeight[GAMELAYERTYPE_VANILLA]-1);

	return ivec2(Nx, Ny);
}

int CCollision::GetPosIndex(int x, int y, int Layer)
{
	return y*m_aWidth[Layer]+x;
}

int CCollision::GetDirFlags(ivec2 Dir)
{
	int Flags = 0;
	if(Dir.x > 0)
		Flags |= DIRFLAG_RIGHT;
	else if(Dir.x < 0)
		Flags |= DIRFLAG_LEFT;

	if(Dir.y > 0)
		Flags |= DIRFLAG_DOWN;
	else if(Dir.y < 0)
		Flags |= DIRFLAG_UP;

	return Flags;
}

int CCollision::GetCollisionAt(float x, float y)
{
	ivec2 Pos = GetTilePos(x, y);
	int Index = GetPosIndex(Pos.x, Pos.y, GAMELAYERTYPE_VANILLA);
	int SwitchGroup = GetSwitchGroup(Index, GAMELAYERTYPE_VANILLA);
	bool Invert = m_apTiles[GAMELAYERTYPE_VANILLA][Index].m_Flags&TILEFLAG_INVERT_SWITCH;
	bool Switch;
	if(SwitchGroup != -1)
		Switch = m_pSwitchStates[SwitchGroup];
	else
		Switch = Invert;
	
	if(Switch == Invert && m_apTiles[GAMELAYERTYPE_VANILLA][Index].m_Index <= 128)
		return m_apTiles[GAMELAYERTYPE_VANILLA][Index].m_Index;
	else
		return 0;
}

int CCollision::GetCollisionMove(float x, float y, float OldX, float OldY, int DirFlagsMask)
{
	ivec2 Pos = GetTilePos(x, y);
	ivec2 OldPos = GetTilePos(OldX, OldY);
	int DirFlags = GetDirFlags(Pos - OldPos)&DirFlagsMask;

	int Index = GetPosIndex(Pos.x, Pos.y, GAMELAYERTYPE_VANILLA);
	int Flags = m_apTiles[GAMELAYERTYPE_VANILLA][Index].m_Flags;
	bool Invert = Flags&TILEFLAG_INVERT_SWITCH;
	int SwitchGroup = GetSwitchGroup(Index, GAMELAYERTYPE_VANILLA);
	bool Switch;
	if(SwitchGroup != -1)
		Switch = m_pSwitchStates[SwitchGroup];
	else
		Switch = Invert;
	
	if(Switch == Invert && m_apTiles[GAMELAYERTYPE_VANILLA][Index].m_Index <= 128 && (Flags&DirFlags) != DirFlags)
		return m_apTiles[GAMELAYERTYPE_VANILLA][Index].m_Index;
	else
		return 0;
}

// TODO: rewrite this smarter!
int CCollision::IntersectLine(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision, int ColFlag)
{
	float Distance = distance(Pos0, Pos1);
	int End(Distance+1);
	vec2 Last = Pos0;

	for(int i = 0; i < End; i++)
	{
		float a = i/Distance;
		vec2 Pos = mix(Pos0, Pos1, a);
		int Col = GetCollisionMove(Pos, Last);
		if(Col&ColFlag)
		{
			if(pOutCollision)
				*pOutCollision = Pos;
			if(pOutBeforeCollision)
				*pOutBeforeCollision = Last;
			return Col;
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
void CCollision::MovePoint(vec2 *pInoutPos, vec2 *pInoutVel, float Elasticity, int *pBounces, int ColFlag)
{
	if(pBounces)
		*pBounces = 0;

	vec2 Pos = *pInoutPos;
	vec2 Vel = *pInoutVel;
	if(GetCollisionMove(Pos + Vel, Pos)&ColFlag)
	{
		int Affected = 0;
		if(GetCollisionMove(Pos.x + Vel.x, Pos.y, Pos)&ColFlag)
		{
			pInoutVel->x *= -Elasticity;
			if(pBounces)
				(*pBounces)++;
			Affected++;
		}

		if(GetCollisionMove(Pos.x, Pos.y + Vel.y, Pos)&ColFlag)
		{
			pInoutVel->y *= -Elasticity;
			if(pBounces)
				(*pBounces)++;
			Affected++;
		}

		if(Affected == 0)
		{
			if(GetCollisionMove(Pos + Vel, Pos.x, Pos.y + Vel.y)&ColFlag)
			{
				pInoutVel->x *= -Elasticity;
				if(pBounces)
					(*pBounces)++;
			}
			if(GetCollisionMove(Pos + Vel, Pos.x + Vel.x, Pos.y)&ColFlag)
			{
				pInoutVel->y *= -Elasticity;
				if(pBounces)
					(*pBounces)++;
			}
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
	if(GetCollisionAt(Pos.x-Size.x, Pos.y-Size.y))
		return true;
	if(GetCollisionAt(Pos.x+Size.x, Pos.y-Size.y))
		return true;
	if(GetCollisionAt(Pos.x-Size.x, Pos.y+Size.y))
		return true;
	if(GetCollisionAt(Pos.x+Size.x, Pos.y+Size.y))
		return true;
	return false;
}

bool CCollision::TestBoxMove(vec2 Pos, vec2 OldPos, vec2 Size)
{
	Size *= 0.5f;
	if(GetCollisionMove(Pos.x-Size.x, Pos.y-Size.y, OldPos.x-Size.x, OldPos.y-Size.y, DIRFLAG_UP|DIRFLAG_LEFT))
		return true;
	if(GetCollisionMove(Pos.x+Size.x, Pos.y-Size.y, OldPos.x+Size.x, OldPos.y-Size.y, DIRFLAG_UP|DIRFLAG_RIGHT))
		return true;
	if(GetCollisionMove(Pos.x-Size.x, Pos.y+Size.y, OldPos.x-Size.x, OldPos.y+Size.y, DIRFLAG_DOWN|DIRFLAG_LEFT))
		return true;
	if(GetCollisionMove(Pos.x+Size.x, Pos.y+Size.y, OldPos.x+Size.x, OldPos.y+Size.y, DIRFLAG_DOWN|DIRFLAG_RIGHT))
		return true;
	return false;
}

bool CCollision::TestHLineMove(vec2 Pos, vec2 OldPos, float Length)
{
	Length *= 0.5f;
	if(GetCollisionMove(Pos.x-Length, Pos.y, OldPos.x-Length, OldPos.y, DIRFLAG_UP|DIRFLAG_DOWN))
		return true;
	if(GetCollisionMove(Pos.x+Length, Pos.y, OldPos.x+Length, OldPos.y, DIRFLAG_UP|DIRFLAG_DOWN))
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
		ivec2 OldPos = GetTilePos(Pos.x, Pos.y);
		bool First = true;
		for(int i = 0; i <= Max; i++)
		{
			//float amount = i/(float)max;
			//if(max == 0)
				//amount = 0;

			vec2 NewPos = Pos + Vel*Fraction; // TODO: this row is not nice

			if(TestBoxMove(NewPos, Pos, Size))
			{
				int Hits = 0;

				if(TestBoxMove(vec2(Pos.x, NewPos.y), Pos, Size))
				{
					NewPos.y = Pos.y;
					Vel.y *= -Elasticity;
					Hits++;
				}

				if(TestBoxMove(vec2(NewPos.x, Pos.y), Pos, Size))
				{
					NewPos.x = Pos.x;
					Vel.x *= -Elasticity;
					Hits++;
				}

				// neither of the tests got a collision.
				// this is a real _corner case_!
				if(Hits == 0)
				{
					if(TestBoxMove(vec2(NewPos.x, NewPos.y), vec2(NewPos.x, Pos.y), Size))
					{
						NewPos.y = Pos.y;
						Vel.y *= -Elasticity;
					}

					if(TestBoxMove(vec2(NewPos.x, NewPos.y), vec2(Pos.x, NewPos.y), Size))
					{
						NewPos.x = Pos.x;
						Vel.x *= -Elasticity;
					}
				}
			}

			Pos = NewPos;

			// speedups
			bool Speedup = false;
			ivec2 iPos = GetTilePos(Pos.x, Pos.y);

			int PosIndex = GetPosIndex(iPos.x, iPos.y, GAMELAYERTYPE_HSPEEDUP);
			if(m_apTiles[GAMELAYERTYPE_HSPEEDUP][PosIndex].m_Index > 0)
			{
				float Accel = m_apTiles[GAMELAYERTYPE_HSPEEDUP][PosIndex].m_Index * Fraction;
				Speedup = true;
				if(m_apTiles[GAMELAYERTYPE_HSPEEDUP][PosIndex].m_Flags&SPEEDUPFLAG_FLIP)
					SpeedupVel.x -= Accel;
				else
					SpeedupVel.x += Accel;
			}

			PosIndex = GetPosIndex(iPos.x, iPos.y, GAMELAYERTYPE_VSPEEDUP);
			if(m_apTiles[GAMELAYERTYPE_VSPEEDUP][PosIndex].m_Index > 0)
			{
				float Accel = m_apTiles[GAMELAYERTYPE_VSPEEDUP][PosIndex].m_Index * Fraction;
				Speedup = true;
				if(m_apTiles[GAMELAYERTYPE_VSPEEDUP][PosIndex].m_Flags&SPEEDUPFLAG_FLIP)
					SpeedupVel.y -= Accel;
				else
					SpeedupVel.y += Accel;
			}

			if(pOutTriggers && (iPos != OldPos || First))
			{
				pOutTriggers[NumTiles] = CTriggers();

				if(Speedup && iPos != OldPos)
					pOutTriggers[NumTiles].m_SpeedupFlags |= TRIGGERFLAG_SPEEDUP;
				HandleTriggerTiles(iPos.x, iPos.y, pOutTriggers + NumTiles);

				// handle teleporters
				int PosIndex = GetPosIndex(iPos.x, iPos.y, GAMELAYERTYPE_TELE);
				int TeleFlags = m_apTiles[GAMELAYERTYPE_TELE][PosIndex].m_Flags;

				if(TeleFlags&TELEFLAG_IN)
				{
					pOutTriggers[NumTiles].m_TeleFlags |= TRIGGERFLAG_TELEPORT;
					pOutTriggers[NumTiles].m_TeleInPos = Pos;

					Pos = m_aTeleTargets[m_apTiles[GAMELAYERTYPE_TELE][PosIndex].m_Index];

					pOutTriggers[NumTiles].m_TeleOutPos = Pos;
					if(TeleFlags&TELEFLAG_RESET_VEL)
					{
						Vel = vec2(0.0f, 0.0f);
						pOutTriggers[NumTiles].m_TeleFlags |= TRIGGERFLAG_STOP_NINJA;
					}
					if(TeleFlags&TELEFLAG_CUT_OTHER)
						pOutTriggers[NumTiles].m_TeleFlags |= TRIGGERFLAG_CUT_OTHER;
					if(TeleFlags&TELEFLAG_CUT_OWN)
						pOutTriggers[NumTiles].m_TeleFlags |= TRIGGERFLAG_CUT_OWN;

					NumTiles++;

					ivec2 iPos = GetTilePos(Pos.x, Pos.y);
					OldPos = iPos;

					pOutTriggers[NumTiles] = CTriggers();
					HandleTriggerTiles(iPos.x, iPos.y, pOutTriggers + NumTiles);
				}
				NumTiles++;

				OldPos = iPos;
			}

			First = false;
		}
	}

	*pInoutPos = Pos;
	*pInoutVel = Vel + SpeedupVel;

	return NumTiles;
}

void CCollision::HandleTriggerTiles(int x, int y, CTriggers *pOutTriggers)
{
	pOutTriggers->m_FreezeFlags = 0;
	int Index = GetPosIndex(x, y, GAMELAYERTYPE_FREEZE);

	if(m_apTiles[GAMELAYERTYPE_FREEZE][Index].m_Index&FREEZEFLAG_FREEZE)
		pOutTriggers->m_FreezeFlags |= TRIGGERFLAG_FREEZE;
	else if(m_apTiles[GAMELAYERTYPE_FREEZE][Index].m_Index&FREEZEFLAG_UNFREEZE)
		pOutTriggers->m_FreezeFlags |= TRIGGERFLAG_UNFREEZE;

	if(m_apTiles[GAMELAYERTYPE_FREEZE][Index].m_Index&FREEZEFLAG_DEEP_FREEZE)
		pOutTriggers->m_FreezeFlags |= TRIGGERFLAG_DEEP_FREEZE;
	else if(m_apTiles[GAMELAYERTYPE_FREEZE][Index].m_Index&FREEZEFLAG_DEEP_UNFREEZE)
		pOutTriggers->m_FreezeFlags |= TRIGGERFLAG_DEEP_UNFREEZE;

	Index = GetPosIndex(x, y, GAMELAYERTYPE_SWITCH);
	if(m_apTiles[GAMELAYERTYPE_SWITCH][Index].m_Index > 0)
	{
		pOutTriggers->m_SwitchFlags |= TRIGGERFLAG_SWITCH;
		pOutTriggers->m_SwitchState = m_apTiles[GAMELAYERTYPE_SWITCH][Index].m_Flags&TILEFLAG_SWITCH_ON;
		pOutTriggers->m_SwitchGroup = m_apTiles[GAMELAYERTYPE_SWITCH][Index].m_Index - 1;
		pOutTriggers->m_SwitchDuration = m_apTiles[GAMELAYERTYPE_SWITCH][Index].m_Reserved;
	}

	Index = GetPosIndex(x, y, GAMELAYERTYPE_RACE);
	if(m_apTiles[GAMELAYERTYPE_RACE][Index].m_Index > 0)
	{
		if(m_apTiles[GAMELAYERTYPE_RACE][Index].m_Index == TILE_RACE_START)
			pOutTriggers->m_Checkpoint = 0;
		else if(m_apTiles[GAMELAYERTYPE_RACE][Index].m_Index == TILE_RACE_FINISH)
			pOutTriggers->m_Checkpoint = m_NumCheckpoints + 1;
		else
			pOutTriggers->m_Checkpoint = m_apTiles[GAMELAYERTYPE_RACE][Index].m_Index - RACE_FIRST_CP_TILE + 1;
	}
	else
		pOutTriggers->m_Checkpoint = -1;
}
