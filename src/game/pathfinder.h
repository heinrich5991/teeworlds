#ifndef GAME_PATHFINDER_H
#define GAME_PATHFINDER_H

#include <base/vmath.h>
#include <base/tl/sorted_array.h>

class CNode
{
public:
    CNode();
    void Init(ivec2 Pos, ivec2 PosTarget, int G = 0);
    ivec2 m_Pos;
    short m_Cost;
    short m_G;
    short m_H;
    char m_State;
    enum
    {
        STATENONE = 0,
        STATEOPENLIST = 1,
        STATECLOSEDLIST = 2,
    };

    CNode *m_pPrev;
};

class CNodePointer
{
public:
    CNode *m_pNode;
    bool operator < (const CNode &Other);
    bool operator < (const CNodePointer &Other);
};

class CPathfinder
{
public:
    CPathfinder(ivec2 Start, ivec2 Target, ivec3 *pMap, int Height, int Width);
    bool Search();
    void Add(CNode *pNode, int x, int y);
    inline int GetCost(int x, int y);
    inline bool OnMap(int x, int y);
private:

    ivec2 m_PosStart;
    ivec2 m_PosTarget;
    CNode *m_pMap;

    int m_Width;
    int m_Height;

    sorted_array<CNodePointer> m_OpenList;
};

#endif
