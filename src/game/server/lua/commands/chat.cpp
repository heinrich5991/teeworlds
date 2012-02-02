/* (c) MAP94 and Patafix. See www.n-lvl.com/ndc/nclient/ for more information. */
#include "../lua.h"
int CLuaFile::SendChatTarget(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (lua_isnumber(L, 1) && lua_isstring(L, 2))
    {		
        pSelf->m_pServer->SendChatTarget(lua_tointeger(L, 1), lua_tostring(L, 2));
    }
    return 0;
}

int CLuaFile::SendChat(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (lua_isnumber(L, 1) && lua_isnumber(L, 2) && lua_isstring(L, 3))
    {
		
        pSelf->m_pServer->SendChat(lua_tointeger(L, 1), lua_tointeger(L, 2), lua_tostring(L, 3));
    }
    return 0;
}