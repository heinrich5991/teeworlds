/* (c) MAP94 and Patafix. See www.n-lvl.com/ndc/nclient/ for more information. */
#include "../lua.h"

int CLuaFile::AddEventListener(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);


    if (!lua_isstring(L, 1) && !lua_isstring(L, 2))
        return 0;
    pSelf->m_pLuaHandler->m_EventListener.AddEventListener(pSelf, (char *)lua_tostring(L, 1), (char *)lua_tostring(L, 2));
    return 0;
}
int CLuaFile::RemoveEventListener(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);


    if (!lua_isstring(L, 1))
        return 0;
    pSelf->m_pLuaHandler->m_EventListener.RemoveEventListener(pSelf, (char *)lua_tostring(L, 1));
    return 0;
}
//Kill-Events
int CLuaFile::KillGetKillerID(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushinteger(L, pSelf->m_pLuaHandler->m_EventListener.m_KillKillerID);
    return 1;
}
int CLuaFile::KillGetVictimID(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushinteger(L, pSelf->m_pLuaHandler->m_EventListener.m_KillVictimID);
    return 1;
}
int CLuaFile::KillGetWeapon(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushinteger(L, pSelf->m_pLuaHandler->m_EventListener.m_KillWeapon);
    return 1;
}
//Chat-Events
int CLuaFile::ChatGetText(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushstring(L, pSelf->m_pLuaHandler->m_EventListener.m_pChatText);
    return 1;
}
int CLuaFile::ChatGetClientID(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushinteger(L, pSelf->m_pLuaHandler->m_EventListener.m_ChatClientID);
    return 1;
}
int CLuaFile::ChatGetTeam(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushinteger(L, pSelf->m_pLuaHandler->m_EventListener.m_ChatTeam);
    return 1;
}
int CLuaFile::ChatHide(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    pSelf->m_pLuaHandler->m_EventListener.m_ChatHide = true;
    return 0;
}