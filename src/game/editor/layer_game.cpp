/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "editor.h"

static const char *const s_apGameLayerTypeNames[] = {
	"Game",
	"Freeze",
};

CLayerGame::CLayerGame(int w, int h, int Type)
: CLayerTiles(w, h)
{
	dbg_assert(0 <= Type && Type < NUM_GAMELAYERTYPES, "game layer type out of range");
	dbg_assert(sizeof(s_apGameLayerTypeNames) / sizeof(s_apGameLayerTypeNames[0]) == NUM_GAMELAYERTYPES, "missing game layer type name");
	str_copy(m_aName, s_apGameLayerTypeNames[Type], sizeof(m_aName));
	m_Game = 1;
	m_GameLayerType = Type;
}

CLayerGame::~CLayerGame()
{
}

int CLayerGame::RenderProperties(CUIRect *pToolbox)
{
	int r = CLayerTiles::RenderProperties(pToolbox);
	m_Image = -1;
	return r;
}
