/* (c) MAP94 and Patafix. See www.n-lvl.com/ndc/nclient/ for more information. */
#include "../lua.h"


int CLuaFile::EntityFind(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4) || !lua_isnumber(L, 5))
        return 0;

    vec2 Pos = vec2(lua_tonumber(L, 1), lua_tonumber(L, 2));

    int Max = clamp((int)lua_tointeger(L, 4), 1, 256);
    CEntity **ppEnt = new CEntity*[Max];

    int Num = pSelf->m_pServer->m_World.FindEntities(Pos, lua_tonumber(L, 3), ppEnt, Max, lua_tointeger(L, 5));
    for (int i = 0; i < Num; i++)
    {
        lua_pushinteger(L, ppEnt[i]->GetID());
    }
    delete []ppEnt;
    return Num;
}

int CLuaFile::EntityGetPos(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1))
        return 0;

    CEntity *pEnt = pSelf->m_pServer->m_World.GetEntityByID(lua_tointeger(L, 1));
    if (pEnt)
    {
        lua_pushnumber(L, pEnt->m_Pos.x);
        lua_pushnumber(L, pEnt->m_Pos.y);
        return 2;
    }

    return 0;
}

int CLuaFile::EntitySetPos(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
        return 0;

    CEntity *pEnt = pSelf->m_pServer->m_World.GetEntityByID(lua_tointeger(L, 1));
    if (pEnt)
    {
        pEnt->m_Pos.x = lua_tonumber(L, 2);
        pEnt->m_Pos.y = lua_tonumber(L, 3);
    }

    return 0;
}

int CLuaFile::EntityDestroy(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1))
        return 0;

    CEntity *pEnt = pSelf->m_pServer->m_World.GetEntityByID(lua_tointeger(L, 1));
    if (pEnt)
    {
        pEnt->Destroy();
    }

    return 0;
}

int CLuaFile::ProjectileFind(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4))
        return 0;

    vec2 Pos = vec2(lua_tonumber(L, 1), lua_tonumber(L, 2));

    int Max = clamp((int)lua_tointeger(L, 4), 1, 256);
    int Num = 0;

    for (CProjectile *pPrj = (CProjectile *)pSelf->m_pServer->m_World.FindFirst(0); pPrj; pPrj = (CProjectile *)pPrj->TypeNext())
    {
		if(distance(pPrj->GetPos((pSelf->m_pServer->Server()->Tick()-pPrj->GetStartTick())/(float)pSelf->m_pServer->Server()->TickSpeed()), Pos) < lua_tonumber(L, 3)+pPrj->m_ProximityRadius)
		{
            lua_pushinteger(L, pPrj->GetID());
			Num++;
			if(Num == Max)
				break;
		}
    }
    return Num;
}

int CLuaFile::ProjectileGetWeapon(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1))
        return 0;

    CProjectile *pPrj = (CProjectile *)pSelf->m_pServer->m_World.GetEntityByID(lua_tointeger(L, 1));
    if (pPrj)
    {
        lua_pushinteger(L, pPrj->GetWeapon());
        return 1;
    }
    return 0;
}

int CLuaFile::ProjectileGetOwner(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1))
        return 0;

    CProjectile *pPrj = (CProjectile *)pSelf->m_pServer->m_World.GetEntityByID(lua_tointeger(L, 1));
    if (pPrj)
    {
        lua_pushinteger(L, pPrj->GetOwner());
        return 1;
    }
    return 0;
}

int CLuaFile::ProjectileGetPos(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1))
        return 0;

    CProjectile *pPrj = (CProjectile *)pSelf->m_pServer->m_World.GetEntityByID(lua_tointeger(L, 1));
    if (pPrj)
    {
        vec2 Pos = pPrj->GetPos((pSelf->m_pServer->Server()->Tick()-pPrj->GetStartTick())/(float)pSelf->m_pServer->Server()->TickSpeed());
        lua_pushnumber(L, Pos.x);
        lua_pushnumber(L, Pos.y);
        return 2;
    }
    return 0;
}

int CLuaFile::ProjectileGetDir(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1))
        return 0;

    CProjectile *pPrj = (CProjectile *)pSelf->m_pServer->m_World.GetEntityByID(lua_tointeger(L, 1));
    if (pPrj)
    {
        vec2 Dir = pPrj->GetDir();
        lua_pushnumber(L, Dir.x);
        lua_pushnumber(L, Dir.y);
        return 2;
    }
    return 0;
}

int CLuaFile::ProjectileGetLifespan(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1))
        return 0;

    CProjectile *pPrj = (CProjectile *)pSelf->m_pServer->m_World.GetEntityByID(lua_tointeger(L, 1));
    if (pPrj)
    {
        lua_pushinteger(L, pPrj->GetLifespan());
        return 1;
    }
    return 0;
}

int CLuaFile::ProjectileGetExplosive(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1))
        return 0;

    CProjectile *pPrj = (CProjectile *)pSelf->m_pServer->m_World.GetEntityByID(lua_tointeger(L, 1));
    if (pPrj)
    {
        lua_pushboolean(L, pPrj->GetExplosive());
        return 1;
    }
    return 0;
}

int CLuaFile::ProjectileGetSoundImpact(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1))
        return 0;

    CProjectile *pPrj = (CProjectile *)pSelf->m_pServer->m_World.GetEntityByID(lua_tointeger(L, 1));
    if (pPrj)
    {
        lua_pushinteger(L, pPrj->GetSoundImpact());
        return 1;
    }
    return 0;
}

int CLuaFile::ProjectileCreate(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2))
        return 0;

    int ID = -1;
    vec2 Dir = vec2(0, -1);
    int Lifespan = 50;
    int Type = WEAPON_GUN;
    int Damage = 1;
    float Force = 0;
    bool Explosive = false;
    int ImpactSound = -1;


    if (lua_isnumber(L, 3) && lua_isnumber(L, 4))
        Dir = vec2(lua_tonumber(L, 3), lua_tonumber(L, 4));
    if (lua_isnumber(L, 5))
        ID = lua_tointeger(L, 5);
    if (lua_isnumber(L, 6))
        Lifespan = lua_tointeger(L, 6);
    if (lua_isnumber(L, 7))
        Type = lua_tointeger(L, 7);
    if (lua_isnumber(L, 8))
        Damage = lua_tointeger(L, 8);
    if (lua_isnumber(L, 9))
        Force = lua_tonumber(L, 9);
    if (lua_isboolean(L, 10))
        Explosive = lua_toboolean(L, 10);
    if (lua_isnumber(L, 11))
        ImpactSound = lua_tonumber(L, 11);

    int Weapon = Type;
    if (lua_isnumber(L, 7))
        Weapon = lua_tointeger(L, 7);

    CProjectile *pProj = new CProjectile(&pSelf->m_pServer->m_World, Type,
        ID,
        vec2(lua_tonumber(L, 1), lua_tonumber(L, 2)),
        Dir,
        Lifespan,
        Damage, Explosive, Force, ImpactSound, Weapon);

    CNetObj_Projectile p;
    pProj->FillInfo(&p);
    return 0;
}

int CLuaFile::CharacterTakeDamage(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1))
        return 0;

    CCharacter *pChr = (CCharacter *)pSelf->m_pServer->m_World.GetEntityByID(lua_tointeger(L, 1));
    if (pChr && pChr->GetType() == CGameWorld::ENTTYPE_CHARACTER)
    {
        int Dmg = 1;
        int From = -1;
        int Weapon = -1;
        if (lua_isnumber(L, 2))
            Dmg = lua_tonumber(L, 2);
        if (lua_isnumber(L, 3))
            From = lua_tonumber(L, 3);
        if (lua_isnumber(L, 4))
            Weapon = lua_tonumber(L, 4);
        pChr->TakeDamage(vec2(0, 0), Dmg, From, Weapon);
    }
    return 0;
}

int CLuaFile::CharacterGetHealth(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1))
        return 0;

    CCharacter *pChr = (CCharacter *)pSelf->m_pServer->m_World.GetEntityByID(lua_tointeger(L, 1));
    if (pChr && pChr->GetType() == CGameWorld::ENTTYPE_CHARACTER)
    {
        lua_pushinteger(L, pChr->GetHealth());
        return 1;
    }
    return 0;
}

int CLuaFile::CharacterGetArmor(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1))
        return 0;

    CCharacter *pChr = (CCharacter *)pSelf->m_pServer->m_World.GetEntityByID(lua_tointeger(L, 1));
    if (pChr && pChr->GetType() == CGameWorld::ENTTYPE_CHARACTER)
    {
        lua_pushinteger(L, pChr->GetHealth());
        return 1;
    }
    return 0;
}

