/* (c) MAP94 and Patafix. See www.n-lvl.com/ndc/nclient/ for more information. */
#include "../lua.h"


int CLuaFile::GetConfigValue(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isstring(L, 1))
        return 0;
/* Client 
    if (str_comp_nocase(lua_tostring(L, 1), "WarningTeambalance") == 0)
    {
        lua_pushinteger(L, g_Config.m_ClWarningTeambalance);
        return 1;
    }
*/

//TODO: Add server side variables
    return 0;
}

int CLuaFile::SetConfigValue(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isstring(L, 1))
        return 0;
    /*if (str_comp_nocase(lua_tostring(L, 1), "WarningTeambalance") == 0 && lua_isnumber(L, 2))
    {
        g_Config.m_ClWarningTeambalance = lua_tointeger(L, 2);
    }*/


//TODO: Add server side variables
    return 0;
}