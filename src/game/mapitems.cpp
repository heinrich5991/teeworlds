/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "mapitems.h"

int TileMapFlagsToGameLayerType(int Flags)
{
	return ((Flags>>GAMELAYERMASK_TYPE_SHIFT)&GAMELAYERMASK_TYPE) - 1;
}

int GameLayerTypeToTileMapFlags(int GameLayerType)
{
	return TILESLAYERFLAG_GAME | (((GameLayerType+1)&GAMELAYERMASK_TYPE)<<GAMELAYERMASK_TYPE_SHIFT);
}
