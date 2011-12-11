/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/graphics.h>
#include <game/mapitems.h>
#include <game/layers.h>
#include "flow.h"

CFlow::CFlow()
{
	m_pCells = 0;
	m_Height = 0;
	m_Width = 0;
	m_Spacing = 16;
	m_Init = false;
}

void CFlow::MapscreenToGroup(float CenterX, float CenterY, CMapItemGroup *pGroup)
{
	float Points[4];
	RenderTools()->MapscreenToWorld(CenterX, CenterY, pGroup->m_ParallaxX/100.0f, pGroup->m_ParallaxY/100.0f,
		pGroup->m_OffsetX, pGroup->m_OffsetY, Graphics()->ScreenAspect(), 1.0f, Points);
	Graphics()->MapScreen(Points[0], Points[1], Points[2], Points[3]);
}

void CFlow::DbgRender()
{
	if(!m_pCells)
		return;

	IGraphics::CLineItem Array[1024];
	int NumItems = 0;
	Graphics()->TextureSet(-1);
	Graphics()->LinesBegin();
	for(int y = 0; y < m_Height; y++)
		for(int x = 0; x < m_Width; x++)
		{
			vec2 Pos(x*m_Spacing, y*m_Spacing);
			vec2 Vel = m_pCells[y*m_Width+x].m_Vel * 0.01f;
            Array[NumItems++] = IGraphics::CLineItem(Pos.x, Pos.y, Pos.x+Vel.x, Pos.y+Vel.y);
			if(NumItems == 1024)
			{
				Graphics()->LinesDraw(Array, 1024);
				NumItems = 0;
			}
		}

	if(NumItems)
		Graphics()->LinesDraw(Array, NumItems);
	Graphics()->LinesEnd();
}

void CFlow::Init()
{
    //return; // fix me //fixed?
    if (m_Init)
        return;
    m_Init = true;
	if(m_pCells)
	{
		mem_free(m_pCells);
		m_pCells = 0;
	}

	CMapItemLayerTilemap *pTilemap = Layers()->GameLayer();
	m_Width = pTilemap->m_Width*32/m_Spacing;
	m_Height = pTilemap->m_Height*32/m_Spacing;

	// allocate and clear
	m_pCells = (CCell *)mem_alloc(sizeof(CCell)*m_Width*m_Height, 1);
	m_pCellsList.set_size(m_Width*m_Height);
	for(int y = 0; y < m_Height; y++)
	{
		for(int x = 0; x < m_Width; x++)
		{
			m_pCells[y*m_Width+x].m_Vel = vec2(0.0f, 0.0f);
			m_pCellsList.clear();
		}
	}
}

void CFlow::Update()
{
    if (Client()->GameTick() && m_Init == false)
        Init();
	if(!m_pCells)
		return;
    for (array<unsigned int>::range r = m_pCellsList.all(); !r.empty(); r.pop_front())
    {
        m_pCells[r.front()].m_Vel *= 0.85f;
        if (length(m_pCells[r.front()].m_Vel) == 0.0f)
        {
            m_pCells[r.front()].m_Vel.x = 0.0f;
            m_pCells[r.front()].m_Vel.y = 0.0f;
            m_pCellsList.remove(r.front());
        }
        else
        {
            //mysterious crash bug.
            /*vec2 Dir = normalize(m_pCells[r.front()].m_Vel);
            float x = Dir.x;
            float y = Dir.y;
            if (Dir.x > 0)
            {
                Add(r.front() + 1, Dir * 0.5f * Dir.x);
            }
            if (Dir.x < 0)
            {
                Add(r.front() - 1, Dir * 0.5f * -Dir.x);
            }
            if (Dir.y > 0)
            {
                Add(r.front() + m_Width, Dir * 0.5f * Dir.y);
            }
            if (Dir.y < 0)
            {
                Add(r.front() - m_Width, Dir * 0.5f * -Dir.y);
            }

            if (Dir.y > 0 && Dir.x > 0)
            {
                Add(r.front() + m_Width + 1, Dir * 0.5f);
            }
            if (Dir.y > 0 && Dir.x < 0)
            {
                Add(r.front() + m_Width - 1, Dir * 0.5f);
            }
            if (Dir.y < 0 && Dir.x > 0)
            {
                Add(r.front() - m_Width + 1, Dir * 0.5f);
            }
            if (Dir.y < 0 && Dir.x < 0)
            {
                Add(r.front() - m_Width - 1, Dir * 0.5f);
            }*/
        }
    }
}

vec2 CFlow::Get(vec2 Pos)
{
	if(!m_pCells)
		return vec2(0,0);

	int x = (int)(Pos.x / m_Spacing);
	int y = (int)(Pos.y / m_Spacing);
	if(x < 0 || y < 0 || x >= m_Width || y >= m_Height)
		return vec2(0,0);

	return m_pCells[y*m_Width+x].m_Vel;
}

void CFlow::Add(vec2 Pos, vec2 Vel, float Size)
{
	if(!m_pCells)
		return;
    if (Size <= 10.0f)
        Size = 0.0f;
    for (float tx = -Size / 10.0f; tx <= Size / 10.0f; tx++)
    {
        for (float ty = -Size / 10.0f; ty <= Size / 10.0f; ty++)
        {
            int x = (int)(Pos.x / m_Spacing) + tx;
            int y = (int)(Pos.y / m_Spacing) + ty;
            vec2 TmpVel = Vel;
            TmpVel.x *= distance(vec2(tx, ty), vec2(0, 0)) - Size * 1.141f;
            TmpVel.y *= distance(vec2(tx, ty), vec2(0, 0)) - Size * 1.141f;
            if(x >= 0 && y >= 0 && x < m_Width && y < m_Height)
            {
                if (length(m_pCells[y*m_Width+x].m_Vel) == 0.0f)
                    m_pCellsList.add(y*m_Width+x);
                m_pCells[y*m_Width+x].m_Vel += Vel;
            }
        }
    }
}

void CFlow::Add(unsigned int i, vec2 Vel)
{
	if(!m_pCells)
		return;
    if(i >= 0 && i < m_Width * m_Height)
    {
        if (length(m_pCells[i].m_Vel) == 0.0f)
            m_pCellsList.add(i);
        m_pCells[i].m_Vel += Vel;
    }
}
