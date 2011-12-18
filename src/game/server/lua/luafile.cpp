/* (c) MAP94 and Patafix. See www.n-lvl.com/ndc/nclient/ for more information. */
#include "lua.h"
/*
#include "components/flow.h"
#include "components/particles.h"
#include "components/menus.h"
#include <game/generated/client_data.h>

#include <engine/serverbrowser.h>
#include <engine/textrender.h>
#include <engine/sound.h>
#include <engine/graphics.h>
#include <engine/storage.h>
#include <engine/server/server.h>

#include <game/client/lineinput.h>
#include <game/client/components/menus.h>
#include <game/client/components/chat.h>
#include <game/client/components/countryflags.h>
#include <game/client/components/skins.h>*/


CLuaFile::CLuaFile()
{
    mem_zero(this, sizeof(CLuaFile));
    Close();
}

CLuaFile::~CLuaFile()
{
    #ifndef CONF_PLATFORM_MACOSX
    End();
    if (m_pLua)
        lua_close(m_pLua);
    m_pLua = 0;
    #endif
}
/*
void CLuaFile::UiTick()
{
    if (!g_Config.m_ClLua)
        return;
    for (int i = 0; i < LUAMAXUIELEMENTS; i++)
    {
        if (m_aUiElements[i].m_Used)
        {
            m_aUiElements[i].Tick();
        }
    }
}
*/
void CLuaFile::Tick()
{
    if (!g_Config.m_SvLua)
        return;

    ErrorFunc(m_pLua);

    FunctionPrepare("Tick");
    PushInteger((int)(time_get() * 10 / time_freq()));
    FunctionExec();

    ErrorFunc(m_pLua);
}
void CLuaFile::TickDefered()
{
    if (!g_Config.m_SvLua)
        return;

    ErrorFunc(m_pLua);

    FunctionPrepare("TickDefered");
    PushInteger((int)(time_get() * 10 / time_freq()));
    FunctionExec();

    ErrorFunc(m_pLua);
}
void CLuaFile::PostTick()
{
    if (!g_Config.m_SvLua)
        return;

    ErrorFunc(m_pLua);

    FunctionPrepare("PostTick");
    PushInteger((int)(time_get() * 10 / time_freq()));
    FunctionExec();

    ErrorFunc(m_pLua);
}

void CLuaFile::End()
{
    if (m_pLua == 0)
        return;

    //try to call the end function
    //Maybe the lua file need to save data eg. a ConfigFile
    FunctionExec("end");
}

int CLuaFile::Panic(lua_State *L)
{
    dbg_break();
    return 0;
}

void CLuaFile::Init(const char *pFile)
{
    //close first
    Close();
   /* //init ui >>not needed server-side<<
    for (int i = 0; i < LUAMAXUIELEMENTS; i++)
        m_aUiElements[i].m_pClient = m_pClient;*/

    str_copy(m_aFilename, pFile, sizeof(m_aFilename));

    m_pLua = lua_open();
    luaL_openlibs(m_pLua);

    lua_atpanic(m_pLua, &Panic);

    //include
    lua_register(m_pLua, "Include", this->Include);

    //Settings
    lua_register(m_pLua, "SetScriptUseSettingPage", this->SetScriptUseSettingPage);
    lua_register(m_pLua, "SetScriptTitle", this->SetScriptTitle);
    lua_register(m_pLua, "SetScriptInfo", this->SetScriptInfo);

    //Eventlistener stuff
    lua_register(m_pLua, "AddEventListener", this->AddEventListener);
    lua_register(m_pLua, "RemoveEventListener", this->RemoveEventListener);

    //menu browser
 /* Client   lua_register(m_pLua, "SetMenuBrowserGameTypeColor", this->SetMenuBrowserGameTypeColor);
    lua_register(m_pLua, "GetMenuBrowserGameTypeName", this->GetMenuBrowserGameTypeName);

    //menu

    lua_register(m_pLua, "MenuActiv", this->MenuActiv);
    lua_register(m_pLua, "MenuGameActiv", this->MenuGameActiv);
    lua_register(m_pLua, "MenuPlayersActiv", this->MenuPlayersActiv);
    lua_register(m_pLua, "MenuServerInfoActiv", this->MenuServerInfoActiv);
    lua_register(m_pLua, "MenuCallVoteActiv", this->MenuCallVoteActiv);
    lua_register(m_pLua, "MenuServersActiv", this->MenuServersActiv);
    lua_register(m_pLua, "MenuMusicActiv", this->MenuMusicActiv);
    lua_register(m_pLua, "MenuDemosActiv", this->MenuDemosActiv);

    //mouse and keyboard
    lua_register(m_pLua, "GetMousePosMenu", this->GetMousePosMenu);
    lua_register(m_pLua, "SetMouseModeRelativ", this->SetMouseModeRelativ);
    lua_register(m_pLua, "SetMouseModeAbsolute", this->SetMouseModeAbsolute);

    //scoreboard
    lua_register(m_pLua, "ScoreboardAbortRender", this->ScoreboardAbortRender);

    //sendinfo
    lua_register(m_pLua, "SendPlayerInfo", this->SendPlayerInfo);
*/
    //Chat
    lua_register(m_pLua, "ChatGetText", this->ChatGetText);
    lua_register(m_pLua, "ChatGetClientID", this->ChatGetClientID);
    lua_register(m_pLua, "ChatGetTeam", this->ChatGetTeam);
    lua_register(m_pLua, "ChatHide", this->ChatHide);

    //Kill
    lua_register(m_pLua, "KillGetKillerID", this->KillGetKillerID);
    lua_register(m_pLua, "KillGetVictimID", this->KillGetVictimID);
    lua_register(m_pLua, "KillGetWeapon", this->KillGetWeapon);

    //Player
    lua_register(m_pLua, "GetPlayerName", this->GetPlayerName);
    lua_register(m_pLua, "GetPlayerClan", this->GetPlayerClan);
    lua_register(m_pLua, "GetPlayerCountry", this->GetPlayerCountry);
    lua_register(m_pLua, "GetPlayerScore", this->GetPlayerScore);
    lua_register(m_pLua, "GetPlayerPing", this->GetPlayerPing);
    lua_register(m_pLua, "GetPlayerTeam", this->GetPlayerTeam);
    lua_register(m_pLua, "GetPlayerSkin", this->GetPlayerSkin);
    lua_register(m_pLua, "GetPlayerColorFeet", this->GetPlayerColorFeet); //Todo: implement me
    lua_register(m_pLua, "GetPlayerColorBody", this->GetPlayerColorBody); //Todo: implement me
    lua_register(m_pLua, "GetPlayerColorSkin", this->GetPlayerColorSkin); //Todo: implement me

    //Emote
    lua_register(m_pLua, "Emote", this->Emote);

    //lua_register(m_pLua, "CreateParticleEmitter", CreateParticleEmitter); //particleemitter gibt es noch nicht
   // lua_register(m_pLua, "CreateParticle", this->CreateParticle);

    //lua_register(m_pLua, "GetFlow", this->GetFlow);
   // lua_register(m_pLua, "SetFlow", this->SetFlow);

    //lua_register(m_pLua, "GetLocalCharacterId", this->GetLocalCharacterId);
    lua_register(m_pLua, "GetCharacterPos", this->GetCharacterPos);
    lua_register(m_pLua, "GetCharacterVel", this->GetCharacterVel);
/*
    //Music
    lua_register(m_pLua, "MusicPlay", this->MusicPlay);
    lua_register(m_pLua, "MusicPause", this->MusicPause);
    lua_register(m_pLua, "MusicStop", this->MusicStop);
    lua_register(m_pLua, "MusicNext", this->MusicNext);
    lua_register(m_pLua, "MusicPrev", this->MusicPrev);
    lua_register(m_pLua, "MusicSetVol", this->MusicSetVol);
    lua_register(m_pLua, "MusicGetVol", this->MusicGetVol);
    lua_register(m_pLua, "MusicGetState", this->MusicGetState);
*/
    lua_register(m_pLua, "GetConfigValue", this->GetConfigValue);
    lua_register(m_pLua, "SetConfigValue", this->SetConfigValue);

    //Console Print
    lua_register(m_pLua, "Print", this->Print);
    lua_register(m_pLua, "Console", this->Console);
/*
    //States
    lua_register(m_pLua, "StateOnline", this->StateOnline);
    lua_register(m_pLua, "StateOffline", this->StateOffline);
    lua_register(m_pLua, "StateConnecting", this->StateConnecting);
    lua_register(m_pLua, "StateDemoplayback", this->StateDemoplayback);
    lua_register(m_pLua, "StateLoading", this->StateLoading);
*/
    //Serverinfo
    lua_register(m_pLua, "GetGameType", this->GetGameType);
    lua_register(m_pLua, "IsTeamplay", this->IsTeamplay);

    //Get Net Error
  //  lua_register(m_pLua, "GetNetError", this->GetNetError);

    //Connect
   // lua_register(m_pLua, "Connect", this->Connect);

    //collision
    lua_register(m_pLua, "IntersectLine", this->IntersectLine);
    lua_register(m_pLua, "GetTile", this->GetTile);
    lua_register(m_pLua, "GetMapWidth", this->GetMapWidth);
    lua_register(m_pLua, "GetMapHeight", this->GetMapHeight);

    //Chat
    lua_register(m_pLua, "SendChat", this->SendChat);
    lua_register(m_pLua, "SendChatTarget", this->SendChatTarget);

    lua_pushlightuserdata(m_pLua, this);
    lua_setglobal(m_pLua, "pLUA");

    lua_register(m_pLua, "errorfunc", this->ErrorFunc); //TODO: fix me
	lua_getglobal(m_pLua, "errorfunc");


    if (luaL_loadfile(m_pLua, m_aFilename) == 0)
    {
        lua_pcall(m_pLua, 0, LUA_MULTRET, 0);
        ErrorFunc(m_pLua);
    }
    lua_getglobal(m_pLua, "errorfunc");
    ErrorFunc(m_pLua);
}

void CLuaFile::Close()
{
    //kill lua
    if (m_pLua)
        lua_close(m_pLua);
    m_pLua = 0;

    //clear
    mem_zero(m_aTitle, sizeof(m_aTitle));
    mem_zero(m_aInfo, sizeof(m_aInfo));
    mem_zero(m_aFilename, sizeof(m_aFilename));
    m_HaveSettings = 0;
    m_FunctionVarNum = 0;
}

int CLuaFile::ErrorFunc(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);

    lua_pop(L,1);

	int depth = 0;
	int frameskip = 1;
	lua_Debug frame;

    if (lua_tostring(L, -1) == 0)
        return 0;

    dbg_msg("Lua", pSelf->m_aFilename);
    dbg_msg("Lua", lua_tostring(L, -1));

    dbg_msg("Lua", "Backtrace:");
    while(lua_getstack(L, depth, &frame) == 1)
    {
        depth++;

        lua_getinfo(L, "nlSf", &frame);

        /* check for functions that just report errors. these frames just confuses more then they help */
        if(frameskip && str_comp(frame.short_src, "[C]") == 0 && frame.currentline == -1)
            continue;
        frameskip = 0;

        /* print stack frame */
        dbg_msg("Lua", "%s(%d): %s %s", frame.short_src, frame.currentline, frame.name, frame.namewhat);
    }
    lua_pop(L, 1); // remove error message
    lua_gc(L, LUA_GCCOLLECT, 0);
    return 0;
}


void CLuaFile::ConfigClose()
{
    FunctionExec("ConfigClose");
}

void CLuaFile::PushString(const char *pString)
{
    if (m_pLua == 0)
        return;
    lua_pushstring(m_pLua, pString);
    m_FunctionVarNum++;
}

void CLuaFile::PushInteger(int value)
{
    if (m_pLua == 0)
        return;
    lua_pushinteger(m_pLua, value);
    m_FunctionVarNum++;
}

void CLuaFile::PushFloat(float value)
{
    if (m_pLua == 0)
        return;
    lua_pushnumber(m_pLua, value);
    m_FunctionVarNum++;
}

void CLuaFile::PushBoolean(bool value)
{
    if (m_pLua == 0)
        return;
    lua_pushboolean(m_pLua, value);
    m_FunctionVarNum++;
}

void CLuaFile::PushParameter(const char *pString)
{
    if (m_pLua == 0)
        return;
    if (StrIsInteger(pString))
    {
        PushInteger(str_toint(pString));
    }
    else if (StrIsFloat(pString))
    {
        PushInteger(str_tofloat(pString));
    }
    else
    {
        PushString(pString);
    }

}

bool CLuaFile::FunctionExist(const char *pFunctionName)
{
    if (m_pLua == 0)
        return false;
    lua_getglobal(m_pLua, pFunctionName);
    return lua_isfunction(m_pLua, lua_gettop(m_pLua));
}

void CLuaFile::FunctionPrepare(const char *pFunctionName)
{
    if (m_pLua == 0 || m_aFilename[0] == 0)
        return;

    lua_pushstring (m_pLua, pFunctionName);
    lua_gettable (m_pLua, LUA_GLOBALSINDEX);
    m_FunctionVarNum = 0;
}

void CLuaFile::FunctionExec(const char *pFunctionName)
{
    if (m_pLua == 0)
        return;
    if (m_aFilename[0] == 0)
        return;

    if (pFunctionName)
    {
        if (FunctionExist(pFunctionName) == false)
            return;
        lua_pushstring (m_pLua, pFunctionName);
        lua_gettable (m_pLua, LUA_GLOBALSINDEX);
    }
    lua_pcall(m_pLua, m_FunctionVarNum, LUA_MULTRET, 0);
    ErrorFunc(m_pLua);
    m_FunctionVarNum = 0;
}


//functions

int CLuaFile::Include(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isstring(L, 1))
        return 0;
    if (luaL_loadfile(L, lua_tostring(L, 1)) == 0)
    {
        lua_pcall(L, 0, LUA_MULTRET, 0);
    }

    return 0;
}

int CLuaFile::SetScriptUseSettingPage(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1))
        return 0;
    pSelf->m_HaveSettings = lua_tointeger(L, 1);
    return 0;
}

int CLuaFile::SetScriptTitle(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isstring(L, 1))
        return 0;
    str_copy(pSelf->m_aTitle, lua_tostring(L, 1), sizeof(pSelf->m_aTitle));
    return 0;
}

int CLuaFile::SetScriptInfo(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isstring(L, 1))
        return 0;
    str_copy(pSelf->m_aInfo, lua_tostring(L, 1), sizeof(pSelf->m_aInfo));
    return 0;
}

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

int CLuaFile::GetPlayerName(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
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
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
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
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
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
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
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
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
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
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
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
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
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
//TODO: Update
int CLuaFile::GetPlayerColorFeet(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    /*if (lua_isnumber(L, 1))
    {
        if (lua_tointeger(L, 1) >= 0 && lua_tointeger(L, 1) < MAX_CLIENTS)
        {
			if(pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)])
			{
				lua_pushnumber(L, pSelf->m_pClient->m_pSkins->GetColorV3(pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)]->m_TeeInfos.m_ColorFeet).r);
				lua_pushnumber(L, pSelf->m_pClient->m_pSkins->GetColorV3(pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)]->m_TeeInfos.m_ColorFeet).g);
				lua_pushnumber(L, pSelf->m_pClient->m_pSkins->GetColorV3(pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)]->m_TeeInfos.m_ColorFeet).b);
				lua_pushnumber(L, 1.0f);
				return 1;
			}
			 TODO: Check if this is needed
            lua_pushnumber(L, pSelf->m_pClient->m_pSkins->GetColorV3(pSelf->m_pClient->m_aClients[lua_tointeger(L, 1)].m_ColorFeet).r);
            lua_pushnumber(L, pSelf->m_pClient->m_pSkins->GetColorV3(pSelf->m_pClient->m_aClients[lua_tointeger(L, 1)].m_ColorFeet).g);
            lua_pushnumber(L, pSelf->m_pClient->m_pSkins->GetColorV3(pSelf->m_pClient->m_aClients[lua_tointeger(L, 1)].m_ColorFeet).b);
            lua_pushnumber(L, 1.0f);
            return 1;
        }
    }*/
    return 0;
}
//TODO: Update
int CLuaFile::GetPlayerColorBody(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    /*if (lua_isnumber(L, 1))
    {
        if (lua_tointeger(L, 1) >= 0 && lua_tointeger(L, 1) < MAX_CLIENTS)
        {
			if(pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)])
			{
				lua_pushnumber(L, pSelf->m_pClient->m_pSkins->GetColorV3(pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)]->m_TeeInfos.m_ColorBody).r);
				lua_pushnumber(L, pSelf->m_pClient->m_pSkins->GetColorV3(pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)]->m_TeeInfos.m_ColorBody).g);
				lua_pushnumber(L, pSelf->m_pClient->m_pSkins->GetColorV3(pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)]->m_TeeInfos.m_ColorBody).b);
				lua_pushnumber(L, 1.0f);
				return 1;
			}
			 TODO: Check if this is needed
            lua_pushnumber(L, pSelf->m_pClient->m_pSkins->GetColorV3(pSelf->m_pClient->m_aClients[lua_tointeger(L, 1)].m_ColorBody).r);
            lua_pushnumber(L, pSelf->m_pClient->m_pSkins->GetColorV3(pSelf->m_pClient->m_aClients[lua_tointeger(L, 1)].m_ColorBody).g);
            lua_pushnumber(L, pSelf->m_pClient->m_pSkins->GetColorV3(pSelf->m_pClient->m_aClients[lua_tointeger(L, 1)].m_ColorBody).b);
            lua_pushnumber(L, 1.0f);
            return 1;
        }
    }*/
    return 0;
}

//TODO: Update
int CLuaFile::GetPlayerColorSkin(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    /*if (lua_isnumber(L, 1))
    {
        if (lua_tointeger(L, 1) >= 0 && lua_tointeger(L, 1) < MAX_CLIENTS)
        {
            if (pSelf->m_pClient->m_aClients[lua_tointeger(L, 1)].m_UseCustomColor)
            {
                lua_pushnumber(L, pSelf->m_pClient->m_pSkins->GetColorV3(pSelf->m_pClient->m_aClients[lua_tointeger(L, 1)].m_ColorBody).r);
                lua_pushnumber(L, pSelf->m_pClient->m_pSkins->GetColorV3(pSelf->m_pClient->m_aClients[lua_tointeger(L, 1)].m_ColorBody).g);
                lua_pushnumber(L, pSelf->m_pClient->m_pSkins->GetColorV3(pSelf->m_pClient->m_aClients[lua_tointeger(L, 1)].m_ColorBody).b);
                lua_pushnumber(L, 1.0f);
                return 1;
            }
            else
            {
                const CSkins::CSkin *s = pSelf->m_pClient->m_pSkins->Get(pSelf->m_pClient->m_aClients[lua_tointeger(L, 1)].m_SkinID);
                if (s)
                {
                    lua_pushnumber(L, s->m_BloodColor.r);
                    lua_pushnumber(L, s->m_BloodColor.g);
                    lua_pushnumber(L, s->m_BloodColor.b);
                    lua_pushnumber(L, 1.0f);
                    return 1;
                }
            }
        }
    }*/
    return 0;
}

int CLuaFile::Console(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (lua_tointeger(L, 1))
    {
            pSelf->m_pServer->Console()->Print(lua_tointeger(L, 1), lua_tostring(L, 2), lua_tostring(L, 3));
    }
    return 0;
}

int CLuaFile::Emote(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (lua_isnumber(L, 1) && lua_isnumber(L, 2) && lua_isnumber(L, 3))
    {
       if(pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)] && pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)]->GetCharacter())
	   {
			pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)]->GetCharacter()->SetEmote(lua_tointeger(L, 2), lua_tointeger(L, 3));
			return 1;
	   }
    }
    return 0;
}


int CLuaFile::GetConfigValue(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isstring(L, 1))
        return 0;
/* Client
    if (str_comp_nocase(lua_tostring(L, 1), "PlayerName") == 0)
    {
        lua_pushstring(L, g_Config.m_PlayerName);
        return 1;
    }
    if (str_comp_nocase(lua_tostring(L, 1), "MouseDeadzone") == 0)
    {
        lua_pushinteger(L, g_Config.m_ClMouseDeadzone);
        return 1;
    }
    if (str_comp_nocase(lua_tostring(L, 1), "MouseFollowfactor") == 0)
    {
        lua_pushinteger(L, g_Config.m_ClMouseFollowfactor);
        return 1;
    }
    if (str_comp_nocase(lua_tostring(L, 1), "MouseMaxDistance") == 0)
    {
        lua_pushinteger(L, g_Config.m_ClMouseMaxDistance);
        return 1;
    }
    if (str_comp_nocase(lua_tostring(L, 1), "PlayerColorBody") == 0)
    {
        lua_pushinteger(L, g_Config.m_PlayerColorBody);
        return 1;
    }
    if (str_comp_nocase(lua_tostring(L, 1), "PlayerColorFeet") == 0)
    {
        lua_pushinteger(L, g_Config.m_PlayerColorFeet);
        return 1;
    }
    if (str_comp_nocase(lua_tostring(L, 1), "Nameplates") == 0)
    {
        lua_pushinteger(L, g_Config.m_ClNameplates);
        return 1;
    }
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
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isstring(L, 1))
        return 0;

//TODO: Add server side variables
    return 0;
}

int CLuaFile::GetCharacterPos(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1))
        return 0;

	 if(pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)] && pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)]->GetCharacter())
    {
        lua_pushnumber(L, pSelf->m_pServer->m_World.m_Core.m_apCharacters[lua_tointeger(L, 1)]->m_Pos.x);
        lua_pushnumber(L, pSelf->m_pServer->m_World.m_Core.m_apCharacters[lua_tointeger(L, 1)]->m_Pos.y);
    }

    lua_pushnumber(L, 0);
    lua_pushnumber(L, 0);
    return 2;
}

int CLuaFile::GetCharacterVel(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1))
        return 0;

    if(pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)] && pSelf->m_pServer->m_apPlayers[lua_tointeger(L, 1)]->GetCharacter())
		{
			lua_pushnumber(L, pSelf->m_pServer->m_World.m_Core.m_apCharacters[lua_tointeger(L, 1)]->m_Vel.x);
			lua_pushnumber(L, pSelf->m_pServer->m_World.m_Core.m_apCharacters[lua_tointeger(L, 1)]->m_Vel.y);
		}
    lua_pushnumber(L, 0);
    lua_pushnumber(L, 0);
    return 2;
}

int CLuaFile::IntersectLine(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    vec2 Pos1 = vec2(lua_tonumber(L, 1), lua_tonumber(L, 2));
    vec2 Pos2 = vec2(lua_tonumber(L, 3), lua_tonumber(L, 4));
    lua_pushnumber(L, pSelf->m_pServer->Collision()->IntersectLine(Pos1, Pos2, 0, 0));
    return 1;
}

int CLuaFile::GetTile(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushnumber(L, pSelf->m_pServer->Collision()->GetTileRaw(lua_tonumber(L, 1), lua_tonumber(L, 2)));
    return 1;
}

int CLuaFile::GetMapWidth(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushnumber(L, pSelf->m_pServer->Collision()->GetWidth());
    return 1;
}

int CLuaFile::GetMapHeight(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushnumber(L, pSelf->m_pServer->Collision()->GetHeight());
    return 1;
}

int CLuaFile::SendChatTarget(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
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
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (lua_isnumber(L, 1) && lua_isnumber(L, 2) && lua_isstring(L, 3))
    {
        pSelf->m_pServer->SendChat(lua_tointeger(L, 1), lua_tointeger(L, 2), lua_tostring(L, 3));
    }
    return 0;
}
/* TODO: Make this server-side
INFO: should we care about particles. should the coder do this itself?
int CLuaFile::CreateParticle(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    CParticle p;
    p.SetDefault();
    if (lua_isnumber(L, 1))
        p.m_Spr = lua_tonumber(L, 1);

    if (lua_isnumber(L, 2))
        p.m_Texture = lua_tonumber(L, 2);

    if (lua_isnumber(L, 3) && lua_isnumber(L, 4))
        p.m_Pos = vec2(lua_tonumber(L, 3), lua_tonumber(L, 4));

    if (lua_isnumber(L, 5) && lua_isnumber(L, 6))
        p.m_Vel = vec2(lua_tonumber(L, 5), lua_tonumber(L, 6));

    if (lua_isnumber(L, 7))
        p.m_LifeSpan = lua_tonumber(L, 7);

    if (lua_isnumber(L, 8))
        p.m_Rot = lua_tonumber(L, 8);

    if (lua_isnumber(L, 9))
        p.m_Rotspeed = lua_tonumber(L, 9);

    if (lua_isnumber(L, 10))
        p.m_StartSize = lua_tonumber(L, 10);

    if (lua_isnumber(L, 11))
        p.m_EndSize = lua_tonumber(L, 11);

    if (lua_isnumber(L, 12))
        p.m_Friction = lua_tonumber(L, 12);

    if (lua_isnumber(L, 13))
        p.m_Gravity.x = lua_tonumber(L, 13);

    if (lua_isnumber(L, 14))
        p.m_Gravity.y = lua_tonumber(L, 14);

    if (lua_isnumber(L, 15))
        p.m_FlowAffected = lua_tonumber(L, 15);

    if (lua_isnumber(L, 16) && lua_isnumber(L, 17) && lua_isnumber(L, 18) && lua_isnumber(L, 19))
        p.m_Color = vec4(lua_tonumber(L, 16), lua_tonumber(L, 17), lua_tonumber(L, 18), lua_tonumber(L, 19));

    if (lua_isnumber(L, 20) && lua_isnumber(L, 21) && lua_isnumber(L, 22) && lua_isnumber(L, 23))
        p.m_ColorEnd = vec4(lua_tonumber(L, 20), lua_tonumber(L, 21), lua_tonumber(L, 22), lua_tonumber(L, 23));

    //Fire ^^
    //CallParent("CreateParticle", 7, 815 - (10-math.random()*20), 1410, 50-math.random()*100, -440, 0.5 + math.random() * 0.5, pi * 10 * (math.random() - 0.5), pi * 2, 36, 0, 0.8, 0, math.random() * -500, 1, 1, 1, 1, 1)
    //CallParent("CreateParticle", 5, 815 - (10-math.random()*20), 1410, 50-math.random()*100, -440, 1 + math.random() * 0.5, pi * 10 * (math.random() - 0.5), pi * 2, 10, 20, 0.8, 0, math.random() * -500, 1, 1, 1, 1, 0.5, 1, 1, 1, 0)
    //CallParent("CreateParticle", 5, 815 - (10-math.random()*20), 1410, 50-math.random()*100, -440, 1 + math.random() * 0.5, pi * 10 * (math.random() - 0.5), pi * 2, 10, 20, 0.8, 0, math.random() * -500, 1, 1, 1, 1, 0.5, 1, 1, 1, 0)

    //lua_pushnumber(L, EmitterId);

	if (pSelf->m_pServer->Server()->GameTick())
        pSelf->m_pServer->m_pParticles->Add(CParticles::GROUP_GENERAL, &p);
    return 0;
}
*/
int CLuaFile::Print(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (lua_isstring(L, 1) && lua_isstring(L, 2))
        dbg_msg(lua_tostring(L, 1), lua_tostring(L, 2));
    return 0;
}

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

int CLuaFile::SendPacket(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)(int)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if(lua_isnil(L, 1) || !lua_isnumber(L, 2))
        return 0;

    char *pData = (char *)lua_tostring(L, 1);
    int Size = str_length(pData);
    CMsgPacker P(NETMSG_LUA_DATA);
    P.AddRaw(pData, Size);
	pSelf->m_pServer->Server()->SendMsgEx(&P, MSGFLAG_VITAL|MSGFLAG_FLUSH, lua_tointeger(L, 2), true);


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

    lua_pushstring(L, pSelf->m_pLuaHandler->m_EventListener.m_pNetData);

    return 1;
}
