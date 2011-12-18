/* (c) MAP94 and Patafix. See www.n-lvl.com/ndc/nclient/ for more information. */
#include "../lua.h"
int CLuaFile::GetGameType(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);   
	if (pSelf->m_pServer->m_pController)
		lua_pushstring(L, pSelf->m_pServer->m_pController->m_pGameType);
	else
		lua_pushstring(L, "");
    return 1;
}

int CLuaFile::IsTeamplay(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);
	
    if (pSelf->m_pServer->m_pController)
        lua_pushboolean(L, pSelf->m_pServer->m_pController->GetGameFlags()&GAMEFLAG_TEAMS);
    else
        lua_pushboolean(L, false);
    return 1;
}