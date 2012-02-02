/* (c) MAP94 and Patafix. See www.n-lvl.com/ndc/nclient/ for more information. */
#include "../lua.h"

int CLuaFile::GetPlayerName(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (lua_isnumber(L, 1))
    {
		if (str_comp(pSelf->m_pServer->Server()->ClientName(lua_tointeger(L, 1)) , "(invalid)") != 0 &&  str_comp(pSelf->m_pServer->Server()->ClientName(lua_tointeger(L, 1)) , "(connecting)") != 0)
			lua_pushstring(L, pSelf->m_pServer->Server()->ClientName(lua_tointeger(L, 1)));
        else
            lua_pushnil(L);      
    }
    else
        lua_pushnil(L);
    return 1;
}

int CLuaFile::GetPlayerClan(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (lua_isnumber(L, 1))
    {
        lua_pushstring(L, pSelf->m_pServer->Server()->ClientClan(lua_tointeger(L, 1)));
        return 1;        
    }
    return 0;
}

int CLuaFile::GetPlayerCountry(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (lua_isnumber(L, 1))
    {
		lua_pushinteger(L, pSelf->m_pServer->Server()->ClientCountry(lua_tointeger(L, 1)));
        return 1;       
    }
    return 0;
}

int CLuaFile::GetPlayerScore(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (lua_isnumber(L, 1))
    {
        if (lua_tointeger(L, 1) >= 0 && lua_tointeger(L, 1) < MAX_CLIENTS)
        {
			if(pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)])
			{
				lua_pushinteger(L, pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)]->m_Score);
				return 1;
			}
            lua_pushinteger(L, 0);
            return 1;
        }
    }
    return 0;
}

int CLuaFile::GetPlayerPing(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (lua_isnumber(L, 1))
    {
        if (lua_tointeger(L, 1) >= 0 && lua_tointeger(L, 1) < MAX_CLIENTS)
        {
            if(pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)])
			{
				lua_pushinteger(L, pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)]->m_Latency.m_Min);
				return 1;
			}    
        }
    }
    return 0;
}

int CLuaFile::GetPlayerTeam(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (lua_isnumber(L, 1))
    {
        if (lua_tointeger(L, 1) >= 0 && lua_tointeger(L, 1) < MAX_CLIENTS)
        {
            if(pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)])
			{
				lua_pushinteger(L, pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)]->GetTeam());
				return 1;
			}
        }
    }
    return 0;
}

int CLuaFile::GetPlayerSkin(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (lua_isnumber(L, 1))
    {
		
        if (lua_tointeger(L, 1) >= 0 && lua_tointeger(L, 1) < MAX_CLIENTS)
        {
            if(pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)])
			{
				lua_pushstring(L, pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)]->m_TeeInfos.m_SkinName);
				return 1;
			}
			lua_pushstring(L, "");
            return 1;
        }
    }
    return 0;
}