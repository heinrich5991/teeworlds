/* (c) MAP94 and Patafix. See www.n-lvl.com/ndc/nclient/ for more information. */
#include "../lua.h"


int CLuaFile::GetCharacterPos(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1))
        return 0;

	 if(lua_tointeger(L, 1) >= 0 && lua_tointeger(L, 1) < MAX_CLIENTS && pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)] && pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)]->GetCharacter())
    {
        lua_pushnumber(L, pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)]->GetCharacter()->m_Pos.x);
        lua_pushnumber(L, pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)]->GetCharacter()->m_Pos.y);
        return 2;
    }

    lua_pushnumber(L, 0);
    lua_pushnumber(L, 0);
    return 2;
}
int CLuaFile::SetCharacterPos(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (lua_isnumber(L, 1) && lua_isnumber(L, 2) && lua_isnumber(L, 3))
    {
		if(lua_tointeger(L, 1) >= 0 && lua_tointeger(L, 1) < MAX_CLIENTS && pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)] && pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)]->GetCharacter())
		{
			pSelf->m_pServer->m_World.m_Core.m_apCharacters[lua_tointeger(L, 1)]->m_Pos.x = lua_tointeger(L, 2);
			pSelf->m_pServer->m_World.m_Core.m_apCharacters[lua_tointeger(L, 1)]->m_Pos.y = lua_tointeger(L, 3);
			return 0;
		}
	}
	return 0;
}

int CLuaFile::GetCharacterVel(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1))
        return 0;

    if(lua_tointeger(L, 1) >= 0 && lua_tointeger(L, 1) < MAX_CLIENTS && pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)] && pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)]->GetCharacter())
    {
        lua_pushnumber(L, pSelf->m_pServer->m_World.m_Core.m_apCharacters[lua_tointeger(L, 1)]->m_Vel.x);
        lua_pushnumber(L, pSelf->m_pServer->m_World.m_Core.m_apCharacters[lua_tointeger(L, 1)]->m_Vel.y);
    }
    lua_pushnumber(L, 0);
    lua_pushnumber(L, 0);
    return 2;
}
int CLuaFile::SetCharacterVel(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (lua_isnumber(L, 1) && lua_isnumber(L, 2) && lua_isnumber(L, 3))
    {
		if(lua_tointeger(L, 1) >= 0 && lua_tointeger(L, 1) < MAX_CLIENTS && pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)] && pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)]->GetCharacter())
		{
			pSelf->m_pServer->m_World.m_Core.m_apCharacters[lua_tointeger(L, 1)]->m_Vel.x = lua_tointeger(L, 2);
			pSelf->m_pServer->m_World.m_Core.m_apCharacters[lua_tointeger(L, 1)]->m_Vel.y = lua_tointeger(L, 3);
			return 0;
		}
	}
	return 0;
}

int CLuaFile::Emote(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (lua_isnumber(L, 1) && lua_isnumber(L, 2) && lua_isnumber(L, 3))
    {
       if(lua_tointeger(L, 1) >= 0 && lua_tointeger(L, 1) < MAX_CLIENTS && pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)] && pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)]->GetCharacter())
	   {
			pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)]->GetCharacter()->SetEmote(lua_tointeger(L, 2), lua_tointeger(L, 3));
			return 0;
	   }
    }
    return 0;
}

int CLuaFile::CharacterSpawn(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (lua_isnumber(L, 1) && lua_isnumber(L, 2) && lua_isnumber(L, 3))
    {
        if(lua_tointeger(L, 1) >= 0 && lua_tointeger(L, 1) < MAX_CLIENTS && pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)])
        {
			pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)]->Spawn(vec2(lua_tonumber(L, 2), lua_tonumber(L, 3)));

			return 0;
        }
    }
    return 0;
}

int CLuaFile::CharacterIsAlive(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (lua_isnumber(L, 1))
    {
        if(lua_tointeger(L, 1) >= 0 && lua_tointeger(L, 1) < MAX_CLIENTS && pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)])
        {
            if (pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)]->GetCharacter())
            {
                lua_pushboolean(L, true);
            }
            else
            {
                lua_pushboolean(L, false);
            }
            return 1;
        }
    }
    return 0;
}

int CLuaFile::IsGrounded(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (lua_isnumber(L, 1))
    {
        if(lua_tointeger(L, 1) >= 0 && lua_tointeger(L, 1) < MAX_CLIENTS && pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)] && pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)]->GetCharacter())
        {
            lua_pushboolean(L, pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)]->GetCharacter()->IsGrounded());
            return 1;
        }
    }
    return 0;
}

int CLuaFile::IncreaseHealth(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (lua_isnumber(L, 1))
    {
        if(lua_tointeger(L, 1) >= 0 && lua_tointeger(L, 1) < MAX_CLIENTS && pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)] && pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)]->GetCharacter())
        {
            int Add = lua_isnumber(L, 2) ? lua_tointeger(L, 2) : 1;
            pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)]->GetCharacter()->IncreaseHealth(Add);
        }
    }
    return 0;
}

int CLuaFile::IncreaseArmor(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (lua_isnumber(L, 1))
    {
        if(lua_tointeger(L, 1) >= 0 && lua_tointeger(L, 1) < MAX_CLIENTS && pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)] && pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)]->GetCharacter())
        {
            int Add = lua_isnumber(L, 2) ? lua_tointeger(L, 2) : 1;
            pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)]->GetCharacter()->IncreaseArmor(Add);
        }
    }
    return 0;
}

int CLuaFile::SetAmmo(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (lua_isnumber(L, 1) && lua_isnumber(L, 2) && lua_tointeger(L, 2) >= 0 && lua_tointeger(L, 2) < NUM_WEAPONS)
    {
        if(lua_tointeger(L, 1) >= 0 && lua_tointeger(L, 1) < MAX_CLIENTS && pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)] && pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)]->GetCharacter())
        {
            pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)]->GetCharacter()->m_aWeapons[lua_tointeger(L, 2)].m_Ammo = lua_isnumber(L, 3) ? lua_tointeger(L, 3) : 10;
            pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)]->GetCharacter()->m_aWeapons[lua_tointeger(L, 2)].m_Got = (pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)]->GetCharacter()->m_aWeapons[lua_tointeger(L, 2)].m_Ammo != 0);
        }
    }
    return 0;
}

