/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_MAPITEMS_H
#define GAME_MAPITEMS_H

// layer types
enum
{
	LAYERTYPE_INVALID=0,
	LAYERTYPE_GAME,
	LAYERTYPE_TILES,
	LAYERTYPE_QUADS,

	MAPITEMTYPE_VERSION=0,
	MAPITEMTYPE_INFO,
	MAPITEMTYPE_IMAGE,
	MAPITEMTYPE_ENVELOPE,
	MAPITEMTYPE_GROUP,
	MAPITEMTYPE_LAYER,
	MAPITEMTYPE_ENVPOINTS,


	CURVETYPE_STEP=0,
	CURVETYPE_LINEAR,
	CURVETYPE_SLOW,
	CURVETYPE_FAST,
	CURVETYPE_SMOOTH,
	NUM_CURVETYPES,

	// game layer tiles
	ENTITY_NULL=0,
	ENTITY_SPAWN,
	ENTITY_SPAWN_RED,
	ENTITY_SPAWN_BLUE,
	ENTITY_FLAGSTAND_RED,
	ENTITY_FLAGSTAND_BLUE,
	ENTITY_ARMOR_1,
	ENTITY_HEALTH_1,
	ENTITY_WEAPON_SHOTGUN,
	ENTITY_WEAPON_GRENADE,
	ENTITY_POWERUP_NINJA,
	ENTITY_WEAPON_LASER,
	ENTITY_NOTHING1,
	ENTITY_NOTHING2,
	ENTITY_NOTHING3,
	ENTITY_NOTHING4,
	ENTITY_NOTHING5,
	ENTITY_CRAZY_BULLET,
	NUM_ENTITIES,

	TILE_AIR=0,
	TILE_SOLID,
	TILE_DEATH,
	TILE_NOHOOK,

	TILE_SEMISOLID_HOOK,
	TILE_SEMISOLID_PROJ,
	TILE_SEMISOLID_PROJ_NOHOOK,
	TILE_SEMISOLID_BOTH,
	
	ROW_NO_OPEN=0,
	ROW_ONE_OPEN,
	ROW_TWO_OPEN,
	ROW_TWO_CORNER_OPEN,
	ROW_THREE_OPEN,

	TILE_FREEZE=1,
	TILE_UNFREEZE,
	TILE_DEEP_FREEZE,
	TILE_DEEP_UNFREEZE,
	TILE_FREEZE_DEEP_UNFREEZE,
	TILE_UNFREEZE_DEEP_UNFREEZE,

	TILE_SWITCH=1,

	TILE_RACE_START=1,
	TILE_RACE_FINISH,
	RACE_FIRST_CP_TILE,

	TILEFLAG_VFLIP=1,
	TILEFLAG_HFLIP=2,
	TILEFLAG_OPAQUE=4,
	TILEFLAG_ROTATE=8,
	TILEFLAG_INVERT_SWITCH=16,

	TILEFLAG_SWITCH_ON=1,

	TELEFLAG_IN=1,
	TELEFLAG_RESET_VEL=2,
	TELEFLAG_CUT_OTHER=4,
	TELEFLAG_CUT_OWN=8,

	SPEEDUPFLAG_FLIP=1,

	LAYERFLAG_DETAIL=1,
	TILESLAYERFLAG_GAME=1,

	GAMELAYERMASK_TYPE=0xF,
	GAMELAYERMASK_TYPE_SHIFT=1,
	GAMELAYERTYPE_VANILLA=0,
	GAMELAYERTYPE_FREEZE,
	GAMELAYERTYPE_SWITCH,
	GAMELAYERTYPE_TELE,
	GAMELAYERTYPE_HSPEEDUP,
	GAMELAYERTYPE_VSPEEDUP,
	GAMELAYERTYPE_RACE,
	NUM_GAMELAYERTYPES,

	ENTITY_OFFSET=255-16*4,
};

struct CPoint
{
	int x, y; // 22.10 fixed point
};

struct CColor
{
	int r, g, b, a;
};

struct CQuad
{
	CPoint m_aPoints[5];
	CColor m_aColors[4];
	CPoint m_aTexcoords[4];

	int m_PosEnv;
	int m_PosEnvOffset;

	int m_ColorEnv;
	int m_ColorEnvOffset;
};

class CTile
{
public:
	unsigned char m_Index;
	unsigned char m_Flags;
	unsigned char m_Skip;
	unsigned char m_Reserved;
};

struct CMapItemInfo
{
	int m_Version;
	int m_Author;
	int m_MapVersion;
	int m_Credits;
	int m_License;
} ;

struct CMapItemImage_v1
{
	int m_Version;
	int m_Width;
	int m_Height;
	int m_External;
	int m_ImageName;
	int m_ImageData;
} ;

struct CMapItemImage : public CMapItemImage_v1
{
	enum { CURRENT_VERSION=2 };
	int m_Format;
};

struct CMapItemGroup_v1
{
	int m_Version;
	int m_OffsetX;
	int m_OffsetY;
	int m_ParallaxX;
	int m_ParallaxY;

	int m_StartLayer;
	int m_NumLayers;
} ;


struct CMapItemGroup : public CMapItemGroup_v1
{
	enum { CURRENT_VERSION=3 };

	int m_UseClipping;
	int m_ClipX;
	int m_ClipY;
	int m_ClipW;
	int m_ClipH;

	int m_aName[3];
} ;

struct CMapItemLayer
{
	int m_Version;
	int m_Type;
	int m_Flags;
} ;

struct CMapItemLayerTilemap
{
	CMapItemLayer m_Layer;
	int m_Version;

	int m_Width;
	int m_Height;
	int m_Flags;

	CColor m_Color;
	int m_ColorEnv;
	int m_ColorEnvOffset;

	int m_Image;
	int m_Data;

	int m_aName[3];
} ;

struct CMapItemLayerQuads
{
	CMapItemLayer m_Layer;
	int m_Version;

	int m_NumQuads;
	int m_Data;
	int m_Image;

	int m_aName[3];
} ;

struct CMapItemVersion
{
	enum { CURRENT_VERSION=1 };

	int m_Version;
} ;

struct CEnvPoint
{
	int m_Time; // in ms
	int m_Curvetype;
	int m_aValues[4]; // 1-4 depending on envelope (22.10 fixed point)

	bool operator<(const CEnvPoint &Other) { return m_Time < Other.m_Time; }
} ;

struct CMapItemEnvelope_v1
{
	int m_Version;
	int m_Channels;
	int m_StartPoint;
	int m_NumPoints;
	int m_aName[8];
} ;

struct CMapItemEnvelope : public CMapItemEnvelope_v1
{
	enum { CURRENT_VERSION=2 };
	int m_Synchronized;
};

#endif
