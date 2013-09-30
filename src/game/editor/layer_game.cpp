/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "editor.h"

static const char *const s_apGameLayerTypeNames[] = {
	"Game",
	"Tele",
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

int CLayerGame::BrushGrab(CLayerGroup *pBrush, CUIRect Rect)
{
	RECTi r;
	Convert(Rect, &r);
	Clamp(&r);

	if(!r.w || !r.h)
		return 0;

	// create new layers
	CLayerGame *pGrabbed = new CLayerGame(r.w, r.h, m_GameLayerType);
	pGrabbed->m_pEditor = m_pEditor;
	pGrabbed->m_Texture = m_Texture;
	pGrabbed->m_Image = m_Image;
	pBrush->AddLayer(pGrabbed);

	// copy the tiles
	for(int y = 0; y < r.h; y++)
		for(int x = 0; x < r.w; x++)
			pGrabbed->m_pTiles[y*pGrabbed->m_Width+x] = m_pTiles[(r.y+y)*m_Width+(r.x+x)];

	return 1;
}

void CLayerGame::BrushToggleTeleIO()
{
	dbg_msg("dbg", "bla1");
	if(m_GameLayerType == GAMELAYERTYPE_TELE)
	{
		dbg_msg("dbg", "bla2");
		CTile *pTempData = new CTile[m_Width*m_Height];
		mem_copy(pTempData, m_pTiles, m_Width*m_Height*sizeof(CTile));
		CTile *pDst = m_pTiles;
		for(int x = 0; x < m_Width; ++x)
			for(int y = m_Height-1; y >= 0; --y, ++pDst)
			{
				*pDst = pTempData[y*m_Width+x];
				pDst->m_Flags ^= TELEFLAG_IN;
			}

		int Temp = m_Width;
		m_Width = m_Height;
		m_Height = Temp;
		delete[] pTempData;
	}
}

int CLayerGame::RenderProperties(CUIRect *pToolbox)
{
	int r = CLayerTiles::RenderProperties(pToolbox);
	m_Image = -1;
	return r;
}
