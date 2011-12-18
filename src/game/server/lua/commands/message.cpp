/* (c) MAP94 and Patafix. See www.n-lvl.com/ndc/nclient/ for more information. */
#include "..\lua.h"

int CLuaFile::SendPacket(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if(lua_isnil(L, 2) || !lua_isnumber(L, 1))
        return 0;

    char *pData = (char *)lua_tostring(L, 2);
    int Size = str_length(pData);
    CMsgPacker P(NETMSG_LUA_DATA);
    P.AddRaw(pData, Size);	
	pSelf->m_pServer->Server()->SendMsgEx(&P, MSGFLAG_VITAL|MSGFLAG_FLUSH, lua_tointeger(L, 1), true);
	

    return 0;
}

int CLuaFile::FetchPacket(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!pSelf->m_pLuaHandler->m_EventListener.m_pNetData)
        return 0;

    return 1;
}


int CLuaFile::AddModFile(lua_State *L)
{
	lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);
	
	if(lua_isnil(L, 1) || !lua_isnumber(L, 2))
        return 0;
	pSelf->m_pServer->Server()->AddModFile((char *)lua_tostring(L, 1), lua_tointeger(L, 2));
	return 1;
}
int CLuaFile::DeleteModFile(lua_State *L)
{
	lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);
	
	if(lua_isnil(L, 1) || !lua_isnumber(L, 2))
        return 0;
	pSelf->m_pServer->Server()->DeleteModFile((char *)lua_tostring(L, 1));
	return 1;
}
int CLuaFile::SendFile(lua_State *L)
{
	lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);
	
	if(!lua_isnumber(L, 1))
        return 0;
	pSelf->m_pServer->Server()->SendFile(lua_tointeger(L, 1));
	return 1;
}

/* TODO: Make this server-side
int CLuaFile::GetNetError(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushstring(L, pSelf->m_pServer->Server()->ErrorString());
    return 1;
}


 TODO: Check this!
int CLuaFile::SendPlayerInfo(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    pSelf->m_pClient->SendInfo(false);
    return 0;
}
*/