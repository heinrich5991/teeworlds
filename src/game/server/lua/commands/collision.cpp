/* (c) MAP94 and Patafix. See www.n-lvl.com/ndc/nclient/ for more information. */
#include "../lua.h"

int CLuaFile::IntersectLine(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    vec2 Pos1 = vec2(lua_tonumber(L, 1), lua_tonumber(L, 2));
    vec2 Pos2 = vec2(lua_tonumber(L, 3), lua_tonumber(L, 4));
    vec2 Out = vec2(0, 0);
    vec2 OutBefore = vec2(0, 0);
    lua_pushnumber(L, pSelf->m_pServer->Collision()->IntersectLine(Pos1, Pos2, &Out, &OutBefore));
    lua_pushnumber(L, Out.x);
    lua_pushnumber(L, Out.y);
    lua_pushnumber(L, OutBefore.x);
    lua_pushnumber(L, OutBefore.y);
    return 5;
}

int CLuaFile::GetTile(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushnumber(L, pSelf->m_pServer->Collision()->GetTileRaw(lua_tonumber(L, 1), lua_tonumber(L, 2)));
    return 1;
}
int CLuaFile::SetTile(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);
	if(!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		return 0;
    pSelf->m_pServer->Collision()->SetTile(lua_tointeger(L, 1), lua_tointeger(L, 2), lua_tointeger(L, 3));
    return 0;
}
int CLuaFile::GetMapWidth(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushnumber(L, pSelf->m_pServer->Collision()->GetWidth());
    return 1;
}

int CLuaFile::GetMapHeight(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushnumber(L, pSelf->m_pServer->Collision()->GetHeight());
    return 1;
}
