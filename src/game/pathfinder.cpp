#include "pathfinder.h"

CNode::CNode()
{
    m_Pos = ivec2(0, 0);
    m_Cost = 0;
    m_G = 0;
    m_H = 0;
    m_State = STATENONE;
    m_pPrev = 0;
}

void CNode::Init(ivec2 Pos, ivec2 PosTarget, int G)
{
    m_G = G;
    m_H = distance(Pos, PosTarget);
}

bool CNodePointer::operator < (const CNode &Other)
{
    if (m_pNode->m_State == CNode::STATEOPENLIST && Other.m_State == CNode::STATEOPENLIST)
    {
        return m_pNode->m_G + m_pNode->m_H + m_pNode->m_Cost < Other.m_G + Other.m_H + Other.m_Cost;
    }
    return false;
}

bool CNodePointer::operator < (const CNodePointer &Other)
{
    if (m_pNode->m_State == CNode::STATEOPENLIST && Other.m_pNode->m_State == CNode::STATEOPENLIST)
    {
        return m_pNode->m_G + m_pNode->m_H + m_pNode->m_Cost < Other.m_pNode->m_G + Other.m_pNode->m_H + Other.m_pNode->m_Cost;
    }
    return false;
}

CPathfinder::CPathfinder(ivec2 Start, ivec2 Target, ivec3 *pMap, int Height, int Width)
{
    m_PosStart = Start;
    m_PosTarget = Target;
    m_Width = Width;
    m_Height = Height;
    m_pMap = new CNode[m_Width * m_Height];
    for (int x = 0; x < m_Width; x++)
    {
        for (int y = 0; y < m_Width; y++)
        {
            m_pMap[x + y * m_Width].m_Pos = ivec2(pMap[x + y * m_Width].x, pMap[x + y * m_Width].y);
            m_pMap[x + y * m_Width].m_Pos = ivec2(pMap[x + y * m_Width].x, pMap[x + y * m_Width].y);
        }
    }

    m_pMap[m_PosStart.x + m_PosStart.y * m_Width].Init(m_PosStart, m_PosTarget);
    CNodePointer Tmp;
    Tmp.m_pNode = &m_pMap[m_PosStart.x + m_PosStart.y * m_Width];
    m_OpenList.add(Tmp);
}

inline int CPathfinder::GetCost(int x, int y)
{
    return m_pMap[x + y * m_Width].m_Cost;
}

inline bool CPathfinder::OnMap(int x, int y)
{
    return x >= 0 && y >= 0 && x < m_Width && y < m_Width;
}

void CPathfinder::Add(CNode *pNode, int x, int y)
{
    if (OnMap(x, y) && m_pMap[x + y * m_Width].m_State != CNode::STATECLOSEDLIST)
    {
        //maybe use the cost of the field we come from?
        int Cost = GetCost(x, y);
        if (Cost == -1) //even change this calulation, see comment above
            return;
        int gCost = Cost + pNode->m_G;
        if (m_pMap[x + y * m_Width].m_State == CNode::STATEOPENLIST)
        {
            if (gCost < m_pMap[x + y * m_Width].m_G)
            {
                m_pMap[x + y * m_Width].m_G = gCost;
                m_pMap[x + y * m_Width].m_pPrev = pNode;
                m_OpenList.sort_range();
            }
        }
        else
        {
            m_pMap[x + y * m_Width].m_G = gCost;
            m_pMap[x + y * m_Width].m_pPrev = pNode;
            m_pMap[x + y * m_Width].m_State = CNode::STATEOPENLIST;
            CNodePointer Tmp;
            Tmp.m_pNode = &m_pMap[x + y * m_Width];
            m_OpenList.add(Tmp);
        }
    }
}

bool CPathfinder::Search()
{
    do
    {
        sorted_array<CNodePointer>::range r = m_OpenList.all();

        int Index = r.size();
        CNode *pCur = r.front().m_pNode;

        if(pCur->m_Pos == m_PosTarget)
        {
            dbg_msg("xD", "\a");
            return true;
        }
        Add(pCur, pCur->m_Pos.x + 1, pCur->m_Pos.y);
        Add(pCur, pCur->m_Pos.x - 1, pCur->m_Pos.y);
        Add(pCur, pCur->m_Pos.x, pCur->m_Pos.y + 1);
        Add(pCur, pCur->m_Pos.x, pCur->m_Pos.y - 1);
        m_OpenList.remove_index(Index);
        m_OpenList.sort_range();
        m_pMap[pCur->m_Pos.x + pCur->m_Pos.y * m_Width].m_State = CNode::STATECLOSEDLIST;
        {
            dbg_msg("x", "%i %i %i %i %i", m_pMap[0].m_State, m_pMap[1].m_State, m_pMap[2].m_State, m_pMap[3].m_State, m_pMap[4].m_State);
            dbg_msg("x", "%i %i %i %i %i", m_pMap[5].m_State, m_pMap[6].m_State, m_pMap[7].m_State, m_pMap[8].m_State, m_pMap[9].m_State);
            dbg_msg("x", "%i %i %i %i %i", m_pMap[10].m_State, m_pMap[11].m_State, m_pMap[12].m_State, m_pMap[13].m_State, m_pMap[14].m_State);
            dbg_msg("x", "%i %i %i %i %i", m_pMap[15].m_State, m_pMap[16].m_State, m_pMap[17].m_State, m_pMap[18].m_State, m_pMap[19].m_State);
            dbg_msg("x", "%i %i %i %i %i", m_pMap[20].m_State, m_pMap[21].m_State, m_pMap[22].m_State, m_pMap[23].m_State, m_pMap[24].m_State);
            dbg_msg("", "");
        }
    } while(m_OpenList.size());
    dbg_msg("-.-", "\a");
    return false;
}
