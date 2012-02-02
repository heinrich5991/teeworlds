/* (c) MAP94. See www.n-lvl.com/ndc/nclient/ for more information. */
#include "lua.h"
#include "components/flow.h"
#include "components/particles.h"

#include <game/generated/client_data.h>

#include <engine/serverbrowser.h>
#include <engine/textrender.h>
#include <engine/sound.h>
#include <engine/graphics.h>
#include <engine/storage.h>
#include <engine/shared/throttle.h>
#include <engine/external/zlib/zlib.h>

#include <game/client/lineinput.h>
#include <game/client/components/menus.h>
#include <game/client/components/chat.h>
#include <game/client/components/countryflags.h>
#include <game/client/components/skins.h>
#include <game/client/components/sounds.h>
#include <game/client/components/stats.h>

CLuaFile::CLuaFile()
{
    mem_zero(this, sizeof(CLuaFile));
    Close();
}

CLuaFile::~CLuaFile()
{
    End();
    #ifndef CONF_PLATFORM_MACOSX
    if (m_pLua)
        lua_close(m_pLua);
    m_pLua = 0;
    #endif
}

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

void CLuaFile::Tick()
{
    if (!g_Config.m_ClLua)
        return;

    ErrorFunc(m_pLua);

    FunctionPrepare("Tick");
    PushInteger((int)(time_get() * 10 / time_freq()));
    PushInteger(m_pClient->m_NewTick);
    PushInteger(m_pClient->m_NewPredictedTick);
    FunctionExec();

    ErrorFunc(m_pLua);
}

void CLuaFile::End()
{
    if (m_pLua == 0)
        return;

    for (array<int>::range r = m_lTextures.all(); !r.empty(); r.pop_front())
    {
        if (g_pData->m_aImages[IMAGE_GAME].m_Id == r.front())
            g_pData->m_aImages[IMAGE_GAME].m_Id = m_pLuaHandler->m_OriginalGameTexture;
        m_pClient->Graphics()->UnloadTexture(r.front());
    }

    m_pLuaHandler->m_EventListener.RemoveAllEventListeners(this);

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
    //init ui
    for (int i = 0; i < LUAMAXUIELEMENTS; i++)
        m_aUiElements[i].m_pClient = m_pClient;

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
    lua_register(m_pLua, "SetMenuBrowserGameTypeColor", this->SetMenuBrowserGameTypeColor);
    lua_register(m_pLua, "GetMenuBrowserGameTypeName", this->GetMenuBrowserGameTypeName);

    //menu these functions are used in some scripts even if the names are wrong. delete this?
    lua_register(m_pLua, "MenuActiv", this->MenuActive);
    lua_register(m_pLua, "MenuGameActiv", this->MenuGameActive);
    lua_register(m_pLua, "MenuPlayersActiv", this->MenuPlayersActive);
    lua_register(m_pLua, "MenuServerInfoActiv", this->MenuServerInfoActive);
    lua_register(m_pLua, "MenuCallVoteActiv", this->MenuCallVoteActive);
    lua_register(m_pLua, "MenuServersActiv", this->MenuServersActive);
    lua_register(m_pLua, "MenuMusicActiv", this->MenuMusicActive);
    lua_register(m_pLua, "MenuDemosActiv", this->MenuDemosActive);

    //menu
    lua_register(m_pLua, "MenuActive", this->MenuActive);
    lua_register(m_pLua, "MenuGameActive", this->MenuGameActive);
    lua_register(m_pLua, "MenuPlayersActive", this->MenuPlayersActive);
    lua_register(m_pLua, "MenuServerInfoActive", this->MenuServerInfoActive);
    lua_register(m_pLua, "MenuCallVoteActive", this->MenuCallVoteActive);
    lua_register(m_pLua, "MenuServersActive", this->MenuServersActive);
    lua_register(m_pLua, "MenuMusicActive", this->MenuMusicActive);
    lua_register(m_pLua, "MenuDemosActive", this->MenuDemosActive);

    //mouse and keyboard
    lua_register(m_pLua, "GetMousePosMenu", this->GetMousePosMenu);
    lua_register(m_pLua, "SetMouseModeRelative", this->SetMouseModeRelative);
    lua_register(m_pLua, "SetMouseModeRelativ", this->SetMouseModeRelative); //pwnd by language
    lua_register(m_pLua, "SetMouseModeAbsolute", this->SetMouseModeAbsolute);

    //scoreboard
    lua_register(m_pLua, "ScoreboardAbortRender", this->ScoreboardAbortRender);

    //sendinfo
    lua_register(m_pLua, "SendPlayerInfo", this->SendPlayerInfo);

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
    lua_register(m_pLua, "GetPlayerColorFeet", this->GetPlayerColorFeet);
    lua_register(m_pLua, "GetPlayerColorBody", this->GetPlayerColorBody);
    lua_register(m_pLua, "GetPlayerColorSkin", this->GetPlayerColorSkin);

    //Emote
    lua_register(m_pLua, "Emote", this->Emote);

    //lua_register(m_pLua, "CreateParticleEmitter", CreateParticleEmitter); //particleemitter gibt es noch nicht
    lua_register(m_pLua, "CreateParticle", this->CreateParticle);

    lua_register(m_pLua, "GetFlow", this->GetFlow);
    lua_register(m_pLua, "SetFlow", this->SetFlow);

    lua_register(m_pLua, "GetLocalCharacterId", this->GetLocalCharacterId);
    lua_register(m_pLua, "GetCharacterPos", this->GetCharacterPos);
    lua_register(m_pLua, "GetCharacterVel", this->GetCharacterVel);

    //Music
    lua_register(m_pLua, "MusicPlay", this->MusicPlay);
    lua_register(m_pLua, "MusicPause", this->MusicPause);
    lua_register(m_pLua, "MusicStop", this->MusicStop);
    lua_register(m_pLua, "MusicNext", this->MusicNext);
    lua_register(m_pLua, "MusicPrev", this->MusicPrev);
    lua_register(m_pLua, "MusicSetVol", this->MusicSetVol);
    lua_register(m_pLua, "MusicGetVol", this->MusicGetVol);
    lua_register(m_pLua, "MusicGetState", this->MusicGetState);

    lua_register(m_pLua, "GetConfigValue", this->GetConfigValue);
    lua_register(m_pLua, "SetConfigValue", this->SetConfigValue);

    lua_register(m_pLua, "GetControlValue", this->GetControlValue);
    lua_register(m_pLua, "SetControlValue", this->SetControlValuePredicted); //for heinrich :*
    lua_register(m_pLua, "SetControlValuePredicted", this->SetControlValuePredicted);
    lua_register(m_pLua, "UnSetControlValue", this->UnSetControlValue);

    //Console Print
    lua_register(m_pLua, "Print", this->Print);
    lua_register(m_pLua, "Console", this->Console);
    
    //Remote console
    lua_register(m_pLua, "RconAuth", this->RconAuth);
    lua_register(m_pLua, "RconAuthed", this->RconAuthed);
    lua_register(m_pLua, "RconExecute", this->RconExecute);

    //States
    lua_register(m_pLua, "StateOnline", this->StateOnline);
    lua_register(m_pLua, "StateOffline", this->StateOffline);
    lua_register(m_pLua, "StateConnecting", this->StateConnecting);
    lua_register(m_pLua, "StateDemoplayback", this->StateDemoplayback);
    lua_register(m_pLua, "StateLoading", this->StateLoading);

    //Serverinfo
    lua_register(m_pLua, "GetGameType", this->GetGameType);
    lua_register(m_pLua, "IsTeamplay", this->IsTeamplay);

    //Get Net Error
    lua_register(m_pLua, "GetNetError", this->GetNetError);

    //Connect
    lua_register(m_pLua, "Connect", this->Connect);

    //collision
    lua_register(m_pLua, "IntersectLine", this->IntersectLine);
    lua_register(m_pLua, "MovePoint", this->MovePoint);
    lua_register(m_pLua, "MoveBox", this->MoveBox);
    lua_register(m_pLua, "GetTile", this->GetTile);
    lua_register(m_pLua, "GetMapWidth", this->GetMapWidth);
    lua_register(m_pLua, "GetMapHeight", this->GetMapHeight);
    lua_register(m_pLua, "SetTile", this->SetTile);
    lua_register(m_pLua, "ClosestPointOnLine", this->ClosestPointOnLine);

	//layer
	lua_register(m_pLua, "GetNumGroups", this->GetNumGroups);
	lua_register(m_pLua, "GetNumLayers", this->GetNumLayers);
	lua_register(m_pLua, "GetGroupNumLayers", this->GetGroupNumLayers);
	lua_register(m_pLua, "GetLayerType", this->GetLayerType);
	lua_register(m_pLua, "GetLayerFlags", this->GetLayerFlags);
	lua_register(m_pLua, "GetLayerTileFlags", this->GetLayerTileFlags);
	lua_register(m_pLua, "GetLayerTileIndex", this->GetLayerTileIndex);
	lua_register(m_pLua, "SetLayerTileFlags", this->SetLayerTileFlags);
	lua_register(m_pLua, "SetLayerTileIndex", this->SetLayerTileIndex);
	lua_register(m_pLua, "GetLayerSize", this->GetLayerSize);
	lua_register(m_pLua, "RenderTilemapGenerateSkip", this->RenderTilemapGenerateSkip);

    //Chat
    lua_register(m_pLua, "ChatSend", this->ChatSend);
    lua_register(m_pLua, "ChatTeamSend", this->ChatTeamSend);

    //Ui
    lua_register(m_pLua, "UiDoButton", this->UiDoButton);
    lua_register(m_pLua, "UiDoEditBox", this->UiDoEditBox);
    lua_register(m_pLua, "UiDoLabel", this->UiDoLabel);
    lua_register(m_pLua, "UiDoRect", this->UiDoRect);
    lua_register(m_pLua, "UiDoImage", this->UiDoImage);
    lua_register(m_pLua, "UiDoImageEx", this->UiDoImageEx);
    lua_register(m_pLua, "UiDoLine", this->UiDoLine);
    lua_register(m_pLua, "UiDoSlider", this->UiDoSlider);
    lua_register(m_pLua, "UiRemoveElement", this->UiRemoveElement);
    lua_register(m_pLua, "UiGetText", this->UiGetText);
    lua_register(m_pLua, "UiSetText", this->UiSetText);
    lua_register(m_pLua, "UiGetColor", this->UiGetColor);
    lua_register(m_pLua, "UiSetColor", this->UiSetColor);
    lua_register(m_pLua, "UiGetRect", this->UiGetRect);
    lua_register(m_pLua, "UiSetRect", this->UiSetRect);
    lua_register(m_pLua, "UiGetScreenWidth", this->UiGetScreenWidth);
    lua_register(m_pLua, "UiGetScreenHeight", this->UiGetScreenHeight);
    lua_register(m_pLua, "UiGetGameTextureID", this->UiGetGameTextureID);
    lua_register(m_pLua, "UiGetParticleTextureID", this->UiGetParticleTextureID);
    lua_register(m_pLua, "UiGetFlagTextureID", this->UiGetFlagTextureID);
    lua_register(m_pLua, "UiDirectRect", this->UiDirectRect);
    lua_register(m_pLua, "BlendNormal", this->BlendNormal);
    lua_register(m_pLua, "BlendAdditive", this->BlendAdditive);

    //

    //Texture
    lua_register(m_pLua, "TextureLoad", this->TextureLoad);
    lua_register(m_pLua, "TextureUnload", this->TextureUnload);
    lua_register(m_pLua, "RenderTexture", this->RenderTexture);
    lua_register(m_pLua, "ReplaceGameTexture", this->ReplaceGameTexture);

    //Net
    lua_register(m_pLua, "FetchPacket", this->FetchPacket);
    lua_register(m_pLua, "SendPacket", this->SendPacket);

    //Sound
    lua_register(m_pLua, "LoadWvFile", this->LoadWvFile);
    lua_register(m_pLua, "PlayWv", this->PlayWv);
    lua_register(m_pLua, "PlaySound", this->PlaySound);

    //keys
    lua_register(m_pLua, "GetKeyFlags", this->GetKeyFlags);
    lua_register(m_pLua, "GetKeyCode", this->GetKeyCode);
    lua_register(m_pLua, "GetKeyUnicode", this->GetKeyUnicode);

    lua_register(m_pLua, "SetLocalCharacterPos", this->SetLocalCharacterPos);

    //demo
    lua_register(m_pLua, "DemoStart", this->DemoStart);
    lua_register(m_pLua, "DemoStop", this->DemoStop);
    lua_register(m_pLua, "DemoDelete", this->DemoDelete);

    //stats
    lua_register(m_pLua, "StatGetNumber", this->StatGetNumber);
    lua_register(m_pLua, "StatGetInfo", this->StatGetInfo);
    lua_register(m_pLua, "StatGetRow", this->StatGetRow);


    lua_register(m_pLua, "TimeGet", this->TimeGet);
    lua_register(m_pLua, "FPS", this->FPS);



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
    mem_zero(m_aUiElements, sizeof(m_aUiElements));
    mem_zero(m_aTitle, sizeof(m_aTitle));
    mem_zero(m_aInfo, sizeof(m_aInfo));
    mem_zero(m_aFilename, sizeof(m_aFilename));
    m_HaveSettings = 0;
    m_FunctionVarNum = 0;
}

int CLuaFile::ErrorFunc(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);

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
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
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
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
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
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
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
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
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
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);


    if (!lua_isstring(L, 1))
        return 0;
    pSelf->m_pLuaHandler->m_EventListener.RemoveEventListener(pSelf, (char *)lua_tostring(L, 1));
    return 0;
}

int CLuaFile::SetMenuBrowserGameTypeColor(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (lua_isnumber(L, 1) && lua_isnumber(L, 2) && lua_isnumber(L, 3) && lua_isnumber(L, 4))
        pSelf->m_pLuaHandler->m_EventListener.m_BrowserActiveGameTypeColor = vec4(lua_tonumber(L, 1), lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4));
    return 0;
}

int CLuaFile::GetMenuBrowserGameTypeName(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushstring(L, pSelf->m_pLuaHandler->m_EventListener.m_pBrowserActiveGameTypeName);
    return 1;
}

int CLuaFile::ScoreboardAbortRender(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    pSelf->m_pLuaHandler->m_EventListener.m_ScoreboardSkipRender = true;
    return 0;
}

int CLuaFile::ChatGetText(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushstring(L, pSelf->m_pLuaHandler->m_EventListener.m_pChatText);
    return 1;
}

int CLuaFile::ChatGetClientID(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushinteger(L, pSelf->m_pLuaHandler->m_EventListener.m_ChatClientID);
    return 1;
}

int CLuaFile::ChatGetTeam(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushinteger(L, pSelf->m_pLuaHandler->m_EventListener.m_ChatTeam);
    return 1;
}

int CLuaFile::ChatHide(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    pSelf->m_pLuaHandler->m_EventListener.m_ChatHide = true;
    return 0;
}

int CLuaFile::KillGetKillerID(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushinteger(L, pSelf->m_pLuaHandler->m_EventListener.m_KillKillerID);
    return 1;
}

int CLuaFile::KillGetVictimID(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushinteger(L, pSelf->m_pLuaHandler->m_EventListener.m_KillVictimID);
    return 1;
}

int CLuaFile::KillGetWeapon(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushinteger(L, pSelf->m_pLuaHandler->m_EventListener.m_KillWeapon);
    return 1;
}

int CLuaFile::GetPlayerName(lua_State *L)
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
            if (pSelf->m_pClient->m_aClients[lua_tointeger(L, 1)].m_aName[0])
                lua_pushstring(L, pSelf->m_pClient->m_aClients[lua_tointeger(L, 1)].m_aName);
            else
                lua_pushnil(L);
        }
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
        if (lua_tointeger(L, 1) >= 0 && lua_tointeger(L, 1) < MAX_CLIENTS)
        {
            lua_pushstring(L, pSelf->m_pClient->m_aClients[lua_tointeger(L, 1)].m_aClan);
            return 1;
        }
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
        if (lua_tointeger(L, 1) >= 0 && lua_tointeger(L, 1) < MAX_CLIENTS)
        {
            lua_pushinteger(L, pSelf->m_pClient->m_aClients[lua_tointeger(L, 1)].m_Country);
            return 1;
        }
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
            const CNetObj_PlayerInfo *pInfo = pSelf->m_pClient->m_Snap.m_paPlayerInfos[lua_tointeger(L, 1)];
            if (pInfo)
            {
                lua_pushinteger(L, pInfo->m_Score);
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
            const CNetObj_PlayerInfo *pInfo = pSelf->m_pClient->m_Snap.m_paPlayerInfos[lua_tointeger(L, 1)];
            if (pInfo)
            {
                lua_pushinteger(L, pInfo->m_Latency);
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
            const CNetObj_PlayerInfo *pInfo = pSelf->m_pClient->m_Snap.m_paPlayerInfos[lua_tointeger(L, 1)];
            if (pInfo)
            {
                lua_pushinteger(L, pInfo->m_Team);
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
            lua_pushstring(L, pSelf->m_pClient->m_aClients[lua_tointeger(L, 1)].m_aSkinName);
            return 1;
        }
    }
    return 0;
}

int CLuaFile::GetPlayerColorFeet(lua_State *L)
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
            lua_pushnumber(L, pSelf->m_pClient->m_pSkins->GetColorV3(pSelf->m_pClient->m_aClients[lua_tointeger(L, 1)].m_ColorFeet).r);
            lua_pushnumber(L, pSelf->m_pClient->m_pSkins->GetColorV3(pSelf->m_pClient->m_aClients[lua_tointeger(L, 1)].m_ColorFeet).g);
            lua_pushnumber(L, pSelf->m_pClient->m_pSkins->GetColorV3(pSelf->m_pClient->m_aClients[lua_tointeger(L, 1)].m_ColorFeet).b);
            lua_pushnumber(L, 1.0f);
            return 1;
        }
    }
    return 0;
}

int CLuaFile::GetPlayerColorBody(lua_State *L)
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
            lua_pushnumber(L, pSelf->m_pClient->m_pSkins->GetColorV3(pSelf->m_pClient->m_aClients[lua_tointeger(L, 1)].m_ColorBody).r);
            lua_pushnumber(L, pSelf->m_pClient->m_pSkins->GetColorV3(pSelf->m_pClient->m_aClients[lua_tointeger(L, 1)].m_ColorBody).g);
            lua_pushnumber(L, pSelf->m_pClient->m_pSkins->GetColorV3(pSelf->m_pClient->m_aClients[lua_tointeger(L, 1)].m_ColorBody).b);
            lua_pushnumber(L, 1.0f);
            return 1;
        }
    }
    return 0;
}

int CLuaFile::GetPlayerColorSkin(lua_State *L)
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
    }
    return 0;
}


int CLuaFile::UiGetScreenWidth(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    CUIRect Screen = *pSelf->m_pClient->UI()->Screen();
    lua_pushnumber(L, Screen.w);
    return 1;
}

int CLuaFile::UiGetScreenHeight(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    CUIRect Screen = *pSelf->m_pClient->UI()->Screen();
    lua_pushnumber(L, Screen.h);
    return 1;
}

int CLuaFile::MusicPlay(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    pSelf->m_pClient->Sound()->m_MusicPlaying = true;
    pSelf->m_pClient->m_Music->m_MusicListActivated = true;
    return 0;
}

int CLuaFile::MusicPause(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    pSelf->m_pClient->Sound()->m_MusicPlaying = pSelf->m_pClient->Sound()->m_MusicPlaying ^ 1;
    if (pSelf->m_pClient->m_Music->m_MusicFirstPlay)
        pSelf->m_pClient->m_Music->m_MusicListActivated = true;
    return 0;
}

int CLuaFile::MusicStop(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    pSelf->m_pClient->Sound()->m_MusicPlaying = false;
    pSelf->m_pClient->m_Music->m_MusicListActivated = false;
    return 0;
}

int CLuaFile::MusicNext(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    pSelf->m_pClient->Sound()->m_MusicPlayIndex++;
    return 0;
}

int CLuaFile::MusicPrev(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    pSelf->m_pClient->Sound()->m_MusicPlayIndex--;
    return 0;
}

int CLuaFile::MusicSetVol(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    g_Config.m_SndMusicVolume = clamp((int)lua_tointeger(L, 1), 0, 100);
    return 0;
}

int CLuaFile::MusicGetVol(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushinteger(L, g_Config.m_SndMusicVolume);
    return 1;
}

int CLuaFile::MusicGetState(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushinteger(L, pSelf->m_pClient->Sound()->m_MusicPlaying);
    return 1;
}

int CLuaFile::MusicGetPlayedIndex(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushinteger(L, pSelf->m_pClient->Sound()->m_MusicPlayIndex);
    return 1;
}

int CLuaFile::SendPlayerInfo(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    pSelf->m_pClient->SendInfo(false);
    return 0;
}

int CLuaFile::Console(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (lua_isnumber(L, 1) && lua_isstring(L, 2) && lua_isstring(L, 3))
    {
        pSelf->m_pClient->Console()->Print(lua_tointeger(L, 1), lua_tostring(L, 2), lua_tostring(L, 3));
    }
    return 0;
}

int CLuaFile::Emote(lua_State *L)
{
    static CThrottle s_Throttle;
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if(s_Throttle.Throttled(5))
    {
        if (lua_isnumber(L, 1))
        {
            CNetMsg_Cl_Emoticon Msg;
            Msg.m_Emoticon = lua_tonumber(L, 1);
            pSelf->m_pClient->Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);
        }
    }
    return 0;
}


int CLuaFile::GetConfigValue(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isstring(L, 1))
        return 0;
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
    return 0;
}

int CLuaFile::SetConfigValue(lua_State *L)
{
    static CThrottle s_Throttle;
    lua_getglobal(L, "pLUA");
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isstring(L, 1))
        return 0;
    if (str_comp_nocase(lua_tostring(L, 1), "PlayerName") == 0 && lua_isstring(L, 2))
    {
        str_copy(g_Config.m_PlayerName, lua_tostring(L, 2), sizeof(g_Config.m_PlayerName));
    }
    if (str_comp_nocase(lua_tostring(L, 1), "MouseDeadzone") == 0 && lua_isnumber(L, 2))
    {
        g_Config.m_ClMouseDeadzone = lua_tointeger(L, 2);
    }
    if (str_comp_nocase(lua_tostring(L, 1), "MouseFollowfactor") == 0 && lua_isnumber(L, 2))
    {
        g_Config.m_ClMouseFollowfactor = lua_tointeger(L, 2);
    }
    if (str_comp_nocase(lua_tostring(L, 1), "MouseMaxDistance") == 0 && lua_isnumber(L, 2))
    {
        g_Config.m_ClMouseMaxDistance = lua_tointeger(L, 2);
    }
    if (str_comp_nocase(lua_tostring(L, 1), "PlayerColorBody") == 0 && lua_isnumber(L, 2))
    {
        if(s_Throttle.Throttled(30))
            g_Config.m_PlayerColorBody = lua_tointeger(L, 2);
    }
    if (str_comp_nocase(lua_tostring(L, 1), "PlayerColorFeet") == 0 && lua_isnumber(L, 2))
    {
        if(s_Throttle.Throttled(30))
            g_Config.m_PlayerColorFeet = lua_tointeger(L, 2);
    }
    if (str_comp_nocase(lua_tostring(L, 1), "Nameplates") == 0 && lua_isnumber(L, 2))
    {
        g_Config.m_ClNameplates = lua_tointeger(L, 2);
    }
    if (str_comp_nocase(lua_tostring(L, 1), "WarningTeambalance") == 0 && lua_isnumber(L, 2))
    {
        g_Config.m_ClWarningTeambalance = lua_tointeger(L, 2);
    }
    return 0;
}

int CLuaFile::GetControlValue(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isstring(L, 1))
        return 0;
    if (str_comp_nocase(lua_tostring(L, 1), "Direction") == 0)
    {
        lua_pushnumber(L, pSelf->m_pClient->m_pLuaBinding->m_ControlDirectionPre);
    }
    if (str_comp_nocase(lua_tostring(L, 1), "Fire") == 0)
    {
        lua_pushnumber(L, pSelf->m_pClient->m_pLuaBinding->m_ControlFirePre);
    }
    if (str_comp_nocase(lua_tostring(L, 1), "Hook") == 0)
    {
        lua_pushnumber(L, pSelf->m_pClient->m_pLuaBinding->m_ControlHookPre);
    }
    if (str_comp_nocase(lua_tostring(L, 1), "Jump") == 0)
    {
        lua_pushnumber(L, pSelf->m_pClient->m_pLuaBinding->m_ControlJumpPre);
    }
    if (str_comp_nocase(lua_tostring(L, 1), "Weapon") == 0)
    {
        lua_pushnumber(L, pSelf->m_pClient->m_pLuaBinding->m_ControlWeaponPre);
    }
    if (str_comp_nocase(lua_tostring(L, 1), "TargetX") == 0)
    {
        lua_pushnumber(L, pSelf->m_pClient->m_pLuaBinding->m_ControlTargetXPre);
    }
    if (str_comp_nocase(lua_tostring(L, 1), "TargetY") == 0)
    {
        lua_pushnumber(L, pSelf->m_pClient->m_pLuaBinding->m_ControlTargetYPre);
    }
    return 1;
}

int CLuaFile::SetControlValue(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isstring(L, 1) && !lua_isnumber(L, 2))
        return 0;
    if (str_comp_nocase(lua_tostring(L, 1), "Direction") == 0)
    {
        pSelf->m_pClient->m_pLuaBinding->m_ControlDirection = lua_tointeger(L, 2);
        pSelf->m_pClient->m_pLuaBinding->m_ControlDirectionIsSet = true;
    }
    if (str_comp_nocase(lua_tostring(L, 1), "Fire") == 0)
    {
        pSelf->m_pClient->m_pLuaBinding->m_ControlFire = lua_tointeger(L, 2);
        pSelf->m_pClient->m_pLuaBinding->m_ControlFireIsSet = true;
    }
    if (str_comp_nocase(lua_tostring(L, 1), "Hook") == 0)
    {
        pSelf->m_pClient->m_pLuaBinding->m_ControlHook = lua_tointeger(L, 2);
        pSelf->m_pClient->m_pLuaBinding->m_ControlHookIsSet = true;
    }
    if (str_comp_nocase(lua_tostring(L, 1), "Jump") == 0)
    {
        pSelf->m_pClient->m_pLuaBinding->m_ControlJump = lua_tointeger(L, 2);
        pSelf->m_pClient->m_pLuaBinding->m_ControlJumpIsSet = true;
    }
    if (str_comp_nocase(lua_tostring(L, 1), "Weapon") == 0)
    {
        pSelf->m_pClient->m_pLuaBinding->m_ControlWeapon = lua_tointeger(L, 2);
        pSelf->m_pClient->m_pLuaBinding->m_ControlWeaponIsSet = true;
    }
    if (str_comp_nocase(lua_tostring(L, 1), "TargetX") == 0)
    {
        pSelf->m_pClient->m_pLuaBinding->m_ControlTargetX = lua_tointeger(L, 2);
        pSelf->m_pClient->m_pLuaBinding->m_ControlTargetXIsSet = true;
    }
    if (str_comp_nocase(lua_tostring(L, 1), "TargetY") == 0)
    {
        pSelf->m_pClient->m_pLuaBinding->m_ControlTargetY = lua_tointeger(L, 2);
        pSelf->m_pClient->m_pLuaBinding->m_ControlTargetYIsSet = true;
    }
    return 0;
}

int CLuaFile::SetControlValuePredicted(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isstring(L, 1) && !lua_isnumber(L, 2))
        return 0;
    if (str_comp_nocase(lua_tostring(L, 1), "Direction") == 0)
    {
        pSelf->m_pClient->m_pLuaBinding->m_ControlDirectionPredicted = lua_tointeger(L, 2);
        pSelf->m_pClient->m_pLuaBinding->m_ControlDirectionPredictedIsSet = true;
    }
    if (str_comp_nocase(lua_tostring(L, 1), "Fire") == 0)
    {
        pSelf->m_pClient->m_pLuaBinding->m_ControlFirePredicted = lua_tointeger(L, 2);
        pSelf->m_pClient->m_pLuaBinding->m_ControlFirePredictedIsSet = true;
    }
    if (str_comp_nocase(lua_tostring(L, 1), "Hook") == 0)
    {
        pSelf->m_pClient->m_pLuaBinding->m_ControlHookPredicted = lua_tointeger(L, 2);
        pSelf->m_pClient->m_pLuaBinding->m_ControlHookPredictedIsSet = true;
    }
    if (str_comp_nocase(lua_tostring(L, 1), "Jump") == 0)
    {
        pSelf->m_pClient->m_pLuaBinding->m_ControlJumpPredicted = lua_tointeger(L, 2);
        pSelf->m_pClient->m_pLuaBinding->m_ControlJumpPredictedIsSet = true;
    }
    if (str_comp_nocase(lua_tostring(L, 1), "Weapon") == 0)
    {
        pSelf->m_pClient->m_pLuaBinding->m_ControlWeaponPredicted = lua_tointeger(L, 2);
        pSelf->m_pClient->m_pLuaBinding->m_ControlWeaponPredictedIsSet = true;
    }
    if (str_comp_nocase(lua_tostring(L, 1), "TargetX") == 0)
    {
        pSelf->m_pClient->m_pLuaBinding->m_ControlTargetXPredicted = lua_tointeger(L, 2);
        pSelf->m_pClient->m_pLuaBinding->m_ControlTargetXPredictedIsSet = true;
    }
    if (str_comp_nocase(lua_tostring(L, 1), "TargetY") == 0)
    {
        pSelf->m_pClient->m_pLuaBinding->m_ControlTargetYPredicted = lua_tointeger(L, 2);
        pSelf->m_pClient->m_pLuaBinding->m_ControlTargetYPredictedIsSet = true;
    }
    return 0;
}

int CLuaFile::UnSetControlValue(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isstring(L, 1))
        return 0;
    if (str_comp_nocase(lua_tostring(L, 1), "Direction") == 0)
    {
        pSelf->m_pClient->m_pLuaBinding->m_ControlDirection = 0;
        pSelf->m_pClient->m_pLuaBinding->m_ControlDirectionPredicted = 0;
        pSelf->m_pClient->m_pLuaBinding->m_ControlDirectionIsSet = false;
        pSelf->m_pClient->m_pLuaBinding->m_ControlDirectionPredictedIsSet = false;
    }
    if (str_comp_nocase(lua_tostring(L, 1), "Fire") == 0)
    {
        pSelf->m_pClient->m_pLuaBinding->m_ControlFire = 0;
        pSelf->m_pClient->m_pLuaBinding->m_ControlFirePredicted = 0;
        pSelf->m_pClient->m_pLuaBinding->m_ControlFireIsSet = false;
        pSelf->m_pClient->m_pLuaBinding->m_ControlFirePredictedIsSet = false;
    }
    if (str_comp_nocase(lua_tostring(L, 1), "Hook") == 0)
    {
        pSelf->m_pClient->m_pLuaBinding->m_ControlHook = 0;
        pSelf->m_pClient->m_pLuaBinding->m_ControlHookPredicted = 0;
        pSelf->m_pClient->m_pLuaBinding->m_ControlHookIsSet = false;
        pSelf->m_pClient->m_pLuaBinding->m_ControlHookPredictedIsSet = false;
    }
    if (str_comp_nocase(lua_tostring(L, 1), "Jump") == 0)
    {
        pSelf->m_pClient->m_pLuaBinding->m_ControlJump = 0;
        pSelf->m_pClient->m_pLuaBinding->m_ControlJumpPredicted = 0;
        pSelf->m_pClient->m_pLuaBinding->m_ControlJumpIsSet = false;
        pSelf->m_pClient->m_pLuaBinding->m_ControlJumpPredictedIsSet = false;
    }
    if (str_comp_nocase(lua_tostring(L, 1), "Weapon") == 0)
    {
        pSelf->m_pClient->m_pLuaBinding->m_ControlWeapon = 0;
        pSelf->m_pClient->m_pLuaBinding->m_ControlWeaponPredicted = 0;
        pSelf->m_pClient->m_pLuaBinding->m_ControlWeaponIsSet = false;
        pSelf->m_pClient->m_pLuaBinding->m_ControlWeaponPredictedIsSet = false;
    }
    if (str_comp_nocase(lua_tostring(L, 1), "TargetX") == 0)
    {
        pSelf->m_pClient->m_pLuaBinding->m_ControlTargetX = 0;
        pSelf->m_pClient->m_pLuaBinding->m_ControlTargetXPredicted = 0;
        pSelf->m_pClient->m_pLuaBinding->m_ControlTargetXIsSet = false;
        pSelf->m_pClient->m_pLuaBinding->m_ControlTargetXPredictedIsSet = false;
    }
    if (str_comp_nocase(lua_tostring(L, 1), "TargetY") == 0)
    {
        pSelf->m_pClient->m_pLuaBinding->m_ControlTargetY = 0;
        pSelf->m_pClient->m_pLuaBinding->m_ControlTargetYPredicted = 0;
        pSelf->m_pClient->m_pLuaBinding->m_ControlTargetYIsSet = false;
        pSelf->m_pClient->m_pLuaBinding->m_ControlTargetYPredictedIsSet = false;
    }
    return 0;
}

int CLuaFile::GetFlow(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    vec2 tmp = pSelf->m_pClient->m_pFlow->Get(vec2(lua_tonumber(L, 1), lua_tonumber(L, 2)));
    lua_pushnumber(L, tmp.x);
    lua_pushnumber(L, tmp.y);
    return 2;
}

int CLuaFile::SetFlow(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    pSelf->m_pClient->m_pFlow->Add(vec2(lua_tonumber(L, 1), lua_tonumber(L, 2)), vec2(lua_tonumber(L, 3), lua_tonumber(L, 4)), lua_tonumber(L, 5));
    return 0;
}

int CLuaFile::GetCharacterPos(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1))
        return 0;
    lua_pushnumber(L, pSelf->m_pClient->m_Snap.m_aCharacters[lua_tointeger(L, 1)].m_Cur.m_X);
    lua_pushnumber(L, pSelf->m_pClient->m_Snap.m_aCharacters[lua_tointeger(L, 1)].m_Cur.m_Y);
    return 2;
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
    lua_pushnumber(L, pSelf->m_pClient->m_Snap.m_aCharacters[lua_tointeger(L, 1)].m_Cur.m_X - pSelf->m_pClient->m_Snap.m_aCharacters[lua_tointeger(L, 1)].m_Prev.m_X);
    lua_pushnumber(L, pSelf->m_pClient->m_Snap.m_aCharacters[lua_tointeger(L, 1)].m_Cur.m_Y - pSelf->m_pClient->m_Snap.m_aCharacters[lua_tointeger(L, 1)].m_Prev.m_Y);
    return 2;
}

int CLuaFile::GetLocalCharacterId(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushnumber(L, pSelf->m_pClient->m_Snap.m_LocalClientID);
    return 1;
}

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
    lua_pushnumber(L, pSelf->m_pClient->Collision()->IntersectLine(Pos1, Pos2, &Out, &OutBefore));
    lua_pushnumber(L, Out.x);
    lua_pushnumber(L, Out.y);
    lua_pushnumber(L, OutBefore.x);
    lua_pushnumber(L, OutBefore.y);
    return 5;
}

int CLuaFile::MovePoint(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1) && !lua_isnumber(L, 2) && !lua_isnumber(L, 3) && !lua_isnumber(L, 4))
        return 0;
    vec2 Pos = vec2(lua_tonumber(L, 1), lua_tonumber(L, 2));
    vec2 Dir = vec2(lua_tonumber(L, 3), lua_tonumber(L, 4));
    float Elasticity = lua_isnumber(L, 5) ? lua_tonumber(L, 5) : 0.0f;
    pSelf->m_pClient->Collision()->MovePoint(&Pos, &Dir, Elasticity, 0);
    lua_pushnumber(L, Pos.x);
    lua_pushnumber(L, Pos.y);
    lua_pushnumber(L, Dir.x);
    lua_pushnumber(L, Dir.y);
    return 4;
}

int CLuaFile::MoveBox(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1) && !lua_isnumber(L, 2) && !lua_isnumber(L, 3) && !lua_isnumber(L, 4) && !lua_isnumber(L, 5) && !lua_isnumber(L, 6))
        return 0;
    vec2 Pos = vec2(lua_tonumber(L, 1), lua_tonumber(L, 2));
    vec2 Dir = vec2(lua_tonumber(L, 3), lua_tonumber(L, 4));
    vec2 Size = vec2(lua_tonumber(L, 5), lua_tonumber(L, 6));
    float Elasticity = lua_isnumber(L, 7) ? lua_tonumber(L, 7) : 0.0f;
    pSelf->m_pClient->Collision()->MoveBox(&Pos, &Dir, Size, Elasticity);
    lua_pushnumber(L, Pos.x);
    lua_pushnumber(L, Pos.y);
    lua_pushnumber(L, Dir.x);
    lua_pushnumber(L, Dir.y);
    return 4;
}

int CLuaFile::ClosestPointOnLine(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1) && !lua_isnumber(L, 2) && !lua_isnumber(L, 3) && !lua_isnumber(L, 4) && !lua_isnumber(L, 5) && !lua_isnumber(L, 6))
        return 0;
    vec2 Pos1 = vec2(lua_tonumber(L, 1), lua_tonumber(L, 2));
    vec2 Pos2 = vec2(lua_tonumber(L, 3), lua_tonumber(L, 4));
    vec2 Pos3 = vec2(lua_tonumber(L, 5), lua_tonumber(L, 6));
    vec2 Ret = closest_point_on_line(Pos1, Pos2, Pos3);
    lua_pushnumber(L, Ret.x);
    lua_pushnumber(L, Ret.y);
    return 2;
}

int CLuaFile::GetTile(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushnumber(L, pSelf->m_pClient->Collision()->GetTileRaw(lua_tonumber(L, 1), lua_tonumber(L, 2)));
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

    pSelf->m_pClient->Collision()->SetTile(lua_tointeger(L, 1), lua_tointeger(L, 2), lua_tointeger(L, 3));
    return 1;
}
int CLuaFile::GetMapWidth(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushnumber(L, pSelf->m_pClient->Collision()->GetWidth());
    return 1;
}

int CLuaFile::GetMapHeight(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushnumber(L, pSelf->m_pClient->Collision()->GetHeight());
    return 1;
}

int CLuaFile::ChatSend(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (lua_isstring(L, 1))
    {
        pSelf->m_pClient->m_pChat->Say(0, lua_tostring(L, 1));
    }
    return 0;
}

int CLuaFile::ChatTeamSend(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (lua_isstring(L, 1))
    {
        pSelf->m_pClient->m_pChat->Say(1, lua_tostring(L, 1));
    }
    return 0;
}

int CLuaFile::CreateParticle(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
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

	if (pSelf->m_pClient->Client()->GameTick())
        pSelf->m_pClient->m_pParticles->Add(CParticles::GROUP_GENERAL, &p);
    return 0;
}

int CLuaFile::Print(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (lua_isstring(L, 1) && lua_isstring(L, 2))
        dbg_msg(lua_tostring(L, 1), lua_tostring(L, 2));
    return 0;
}

int CLuaFile::RconAuth(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (lua_isstring(L, 1))
        pSelf->m_pClient->Client()->RconAuth("", lua_tostring(L, 1));

    return 0;
}

int CLuaFile::RconAuthed(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushboolean(L, pSelf->m_pClient->Client()->RconAuthed());

    return 1;
}

int CLuaFile::RconExecute(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (lua_isstring(L, 1))
        pSelf->m_pClient->Client()->Rcon(lua_tostring(L, 1));

    return 0;
}

int CLuaFile::GetGameType(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    CServerInfo CurrentServerInfo;
	pSelf->m_pClient->Client()->GetServerInfo(&CurrentServerInfo);
    lua_pushstring(L, CurrentServerInfo.m_aGameType);
    return 1;
}

int CLuaFile::IsTeamplay(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (pSelf->m_pClient->m_Snap.m_pGameInfoObj)
        lua_pushboolean(L, pSelf->m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_TEAMS);
    else
        lua_pushboolean(L, false);
    return 1;
}

int CLuaFile::GetNetError(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushstring(L, pSelf->m_pClient->Client()->ErrorString());
    return 1;
}

int CLuaFile::Connect(lua_State *L)
{
    static CThrottle s_Throttle;
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if(s_Throttle.Throttled(5))
    {
        if (lua_isstring(L, 1))
            pSelf->m_pClient->Client()->Connect(lua_tostring(L, 1));
        else
            pSelf->m_pClient->Client()->Connect(g_Config.m_UiServerAddress);
    }
    return 0;
}

int CLuaFile::StateGetOld(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushboolean(L, pSelf->m_pLuaHandler->m_EventListener.m_StateOld);
    return 1;
}

int CLuaFile::StateGet(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushboolean(L, pSelf->m_pClient->Client()->State());
    return 1;
}

int CLuaFile::StateOnline(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushboolean(L, pSelf->m_pClient->Client()->State() == IClient::STATE_ONLINE);
    return 1;
}

int CLuaFile::StateOffline(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushboolean(L, pSelf->m_pClient->Client()->State() == IClient::STATE_OFFLINE);
    return 1;
}

int CLuaFile::StateConnecting(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushboolean(L, pSelf->m_pClient->Client()->State() == IClient::STATE_CONNECTING);
    return 1;
}

int CLuaFile::StateLoading(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushboolean(L, pSelf->m_pClient->Client()->State() == IClient::STATE_LOADING);
    return 1;
}

int CLuaFile::StateDemoplayback(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushboolean(L, pSelf->m_pClient->Client()->State() == IClient::STATE_DEMOPLAYBACK);
    return 1;
}

int CLuaFile::MenuActive(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushboolean(L, pSelf->m_pClient->m_pMenus->IsActive());
    return 1;
}

int CLuaFile::MenuGameActive(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushboolean(L, pSelf->m_pClient->m_pMenus->IsActive() && pSelf->m_pClient->m_pMenus->GetGamePage() == CMenus::PAGE_GAME);
    return 1;
}

int CLuaFile::MenuPlayersActive(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushboolean(L, pSelf->m_pClient->m_pMenus->IsActive() && pSelf->m_pClient->m_pMenus->GetGamePage() == CMenus::PAGE_PLAYERS);
    return 1;
}

int CLuaFile::MenuServerInfoActive(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushboolean(L, pSelf->m_pClient->m_pMenus->IsActive() && pSelf->m_pClient->m_pMenus->GetGamePage() == CMenus::PAGE_SERVER_INFO);
    return 1;
}

int CLuaFile::MenuCallVoteActive(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushboolean(L, pSelf->m_pClient->m_pMenus->IsActive() && pSelf->m_pClient->m_pMenus->GetGamePage() == CMenus::PAGE_CALLVOTE);
    return 1;
}

int CLuaFile::MenuServersActive(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushboolean(L, pSelf->m_pClient->m_pMenus->IsActive() && pSelf->m_pClient->m_pMenus->GetGamePage() == CMenus::PAGE_SERVERS);
    return 1;
}

int CLuaFile::MenuMusicActive(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushboolean(L, pSelf->m_pClient->m_pMenus->IsActive() && pSelf->m_pClient->m_pMenus->GetGamePage() == CMenus::PAGE_MUSIC);
    return 1;
}

int CLuaFile::MenuDemosActive(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushboolean(L, pSelf->m_pClient->m_pMenus->IsActive() && pSelf->m_pClient->m_pMenus->GetGamePage() == CMenus::PAGE_DEMOS);
    return 1;
}

int CLuaFile::GetMousePosMenu(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    //convert the pos to a ui mouse pos
    lua_pushnumber(L, pSelf->m_pClient->m_pMenus->GetMousePos().x * pSelf->m_pClient->UI()->Screen()->w / pSelf->m_pClient->Graphics()->ScreenWidth());
    lua_pushnumber(L, pSelf->m_pClient->m_pMenus->GetMousePos().y * pSelf->m_pClient->UI()->Screen()->h / pSelf->m_pClient->Graphics()->ScreenHeight());
    return 2;
}

int CLuaFile::SetMouseModeRelative(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    pSelf->m_pClient->m_pLua->m_MouseModeAbsolute = false;
    return 1;
}

int CLuaFile::SetMouseModeAbsolute(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    pSelf->m_pClient->m_pLua->m_MouseModeAbsolute = true;
    return 1;
}

int CLuaFile::UiRemoveElement(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1))
        return 0;

    int i = lua_tonumber(L, 1);
    if (i >= 0 && i < LUAMAXUIELEMENTS)
    {
        pSelf->m_aUiElements[i].m_Used = false;
    }
    return 0;
}

int CLuaFile::UiGetText(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1))
        return 0;

    int i = lua_tonumber(L, 1);
    if (i >= 0 && i < LUAMAXUIELEMENTS)
    {
        lua_pushstring(L, pSelf->m_aUiElements[i].m_pText);
        return 1;
    }
    return 0;
}

int CLuaFile::UiSetText(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1))
        return 0;
    if (!lua_isstring(L, 2))
        return 0;

    int i = lua_tonumber(L, 1);
    if (i >= 0 && i < LUAMAXUIELEMENTS)
    {
        str_copy(pSelf->m_aUiElements[i].m_pText, lua_tostring(L, 2), sizeof(pSelf->m_aUiElements[i].m_pText));
    }
    return 0;
}

int CLuaFile::UiGetColor(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1))
        return 0;

    int i = lua_tonumber(L, 1);
    if (i >= 0 && i < LUAMAXUIELEMENTS)
    {
        lua_pushnumber(L, pSelf->m_aUiElements[i].m_Color.r);
        lua_pushnumber(L, pSelf->m_aUiElements[i].m_Color.g);
        lua_pushnumber(L, pSelf->m_aUiElements[i].m_Color.b);
        lua_pushnumber(L, pSelf->m_aUiElements[i].m_Color.a);
        return 4;
    }
    return 0;
}

int CLuaFile::UiSetColor(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1))
        return 0;
    if (!lua_isnumber(L, 2))
        return 0;
    if (!lua_isnumber(L, 3))
        return 0;
    if (!lua_isnumber(L, 4))
        return 0;
    if (!lua_isnumber(L, 5))
        return 0;

    int i = lua_tonumber(L, 1);
    if (i >= 0 && i < LUAMAXUIELEMENTS)
    {
        pSelf->m_aUiElements[i].m_Color.r = lua_tonumber(L, 2);
        pSelf->m_aUiElements[i].m_Color.g = lua_tonumber(L, 3);
        pSelf->m_aUiElements[i].m_Color.b = lua_tonumber(L, 4);
        pSelf->m_aUiElements[i].m_Color.a = lua_tonumber(L, 5);
    }
    return 0;
}

int CLuaFile::UiGetRect(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1))
        return 0;

    int i = lua_tonumber(L, 1);
    if (i >= 0 && i < LUAMAXUIELEMENTS)
    {
        lua_pushnumber(L, pSelf->m_aUiElements[i].m_Rect.x);
        lua_pushnumber(L, pSelf->m_aUiElements[i].m_Rect.y);
        lua_pushnumber(L, pSelf->m_aUiElements[i].m_Rect.w);
        lua_pushnumber(L, pSelf->m_aUiElements[i].m_Rect.h);
        return 4;
    }
    return 0;
}

int CLuaFile::UiSetRect(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1))
        return 0;
    if (!lua_isnumber(L, 2))
        return 0;
    if (!lua_isnumber(L, 3))
        return 0;
    if (!lua_isnumber(L, 4))
        return 0;
    if (!lua_isnumber(L, 5))
        return 0;

    int i = lua_tonumber(L, 1);
    if (i >= 0 && i < LUAMAXUIELEMENTS)
    {
        pSelf->m_aUiElements[i].m_Rect.x = lua_tonumber(L, 2);
        pSelf->m_aUiElements[i].m_Rect.y = lua_tonumber(L, 3);
        pSelf->m_aUiElements[i].m_Rect.w = lua_tonumber(L, 4);
        pSelf->m_aUiElements[i].m_Rect.h = lua_tonumber(L, 5);
    }
    return 0;
}

int CLuaFile::UiDoButton(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1))
        return 0;
    if (!lua_isnumber(L, 2))
        return 0;
    if (!lua_isnumber(L, 3))
        return 0;
    if (!lua_isnumber(L, 4))
        return 0;
    if (!lua_isnumber(L, 5))
        return 0;
    if (!lua_isstring(L, 6))
        return 0;
    if (!lua_isstring(L, 7))
        return 0;

    int i = 0;
    for (i = 0; i < LUAMAXUIELEMENTS; i++)
    {
        if (pSelf->m_aUiElements[i].m_Used == 0)
        {
            break;
        }
    }
    if (i >= LUAMAXUIELEMENTS)
        return 0;

    pSelf->m_aUiElements[i].m_Used = true;
    pSelf->m_aUiElements[i].m_Rect.x = lua_tonumber(L, 1);
    pSelf->m_aUiElements[i].m_Rect.y = lua_tonumber(L, 2);
    pSelf->m_aUiElements[i].m_Rect.w = lua_tonumber(L, 3);
    pSelf->m_aUiElements[i].m_Rect.h = lua_tonumber(L, 4);
    pSelf->m_aUiElements[i].m_RegPoint = lua_tonumber(L, 5);
    str_copy(pSelf->m_aUiElements[i].m_pText, lua_tostring(L, 6), sizeof(pSelf->m_aUiElements[i].m_pText));
    str_copy(pSelf->m_aUiElements[i].m_pCallback, lua_tostring(L, 7), sizeof(pSelf->m_aUiElements[i].m_pCallback));
    if (lua_isnumber(L, 8))
        pSelf->m_aUiElements[i].m_Checked = lua_tonumber(L, 8);
    else
        pSelf->m_aUiElements[i].m_Checked = 0;

    if (lua_isnumber(L, 9))
        pSelf->m_aUiElements[i].m_Corners = lua_tonumber(L, 9);
    else
        pSelf->m_aUiElements[i].m_Corners = CUI::CORNER_ALL;

    if (lua_isnumber(L, 10) && lua_isnumber(L, 11) && lua_isnumber(L, 12) && lua_isnumber(L, 13))
        pSelf->m_aUiElements[i].m_Color = vec4(lua_tonumber(L, 10), lua_tonumber(L, 11), lua_tonumber(L, 12), lua_tonumber(L, 13));
    else
        pSelf->m_aUiElements[i].m_Color = vec4(1.0f, 1.0f, 1.0f, 0.5f);

    pSelf->m_aUiElements[i].m_pClient = pSelf->m_pClient;
    pSelf->m_aUiElements[i].m_pLuaFile = pSelf;
    pSelf->m_aUiElements[i].m_Type = CLuaUi::LUAUIBUTTON;

    lua_pushinteger(L, i);

    return 1;
}

int CLuaFile::UiDoEditBox(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1))
        return 0;
    if (!lua_isnumber(L, 2))
        return 0;
    if (!lua_isnumber(L, 3))
        return 0;
    if (!lua_isnumber(L, 4))
        return 0;
    if (!lua_isnumber(L, 5))
        return 0;

    int i = 0;
    for (i = 0; i < LUAMAXUIELEMENTS; i++)
    {
        if (pSelf->m_aUiElements[i].m_Used == 0)
        {
            break;
        }
    }
    if (i >= LUAMAXUIELEMENTS)
        return 0;

    pSelf->m_aUiElements[i].m_Used = true;
    pSelf->m_aUiElements[i].m_Rect.x = lua_tonumber(L, 1);
    pSelf->m_aUiElements[i].m_Rect.y = lua_tonumber(L, 2);
    pSelf->m_aUiElements[i].m_Rect.w = lua_tonumber(L, 3);
    pSelf->m_aUiElements[i].m_Rect.h = lua_tonumber(L, 4);
    pSelf->m_aUiElements[i].m_RegPoint = lua_tonumber(L, 5);

    if (lua_isstring(L, 6))
        str_copy(pSelf->m_aUiElements[i].m_pText, lua_tostring(L, 6), sizeof(pSelf->m_aUiElements[i].m_pText));
    else
        pSelf->m_aUiElements[i].m_pText[0] = 0;

    if (lua_isnumber(L, 7))
        pSelf->m_aUiElements[i].m_FontSize = lua_tonumber(L, 7);
    else
        pSelf->m_aUiElements[i].m_FontSize = 14.0f;

    if (lua_isnumber(L, 8))
        pSelf->m_aUiElements[i].m_Hidden = lua_toboolean(L, 8);
    else
        pSelf->m_aUiElements[i].m_Hidden = false;

    if (lua_isnumber(L, 9))
        pSelf->m_aUiElements[i].m_Corners = lua_tonumber(L, 9);
    else
        pSelf->m_aUiElements[i].m_Corners = CUI::CORNER_ALL;

    if (lua_isnumber(L, 10) && lua_isnumber(L, 11) && lua_isnumber(L, 12) && lua_isnumber(L, 13))
        pSelf->m_aUiElements[i].m_Color = vec4(lua_tonumber(L, 10), lua_tonumber(L, 11), lua_tonumber(L, 12), lua_tonumber(L, 13));
    else
        pSelf->m_aUiElements[i].m_Color = vec4(1.0f, 1.0f, 1.0f, 0.5f);

    pSelf->m_aUiElements[i].m_pClient = pSelf->m_pClient;
    pSelf->m_aUiElements[i].m_pLuaFile = pSelf;
    pSelf->m_aUiElements[i].m_Type = CLuaUi::LUAUIEDITBOX;

    lua_pushinteger(L, i);

    return 1;
}

int CLuaFile::UiDoLabel(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1))
        return 0;
    if (!lua_isnumber(L, 2))
        return 0;
    if (!lua_isnumber(L, 3))
        return 0;
    if (!lua_isnumber(L, 4))
        return 0;
    if (!lua_isnumber(L, 5))
        return 0;


    int i = 0;
    for (i = 0; i < LUAMAXUIELEMENTS; i++)
    {
        if (pSelf->m_aUiElements[i].m_Used == 0)
        {
            break;
        }
    }
    if (i >= LUAMAXUIELEMENTS)
        return 0;

    pSelf->m_aUiElements[i].m_Used = true;
    pSelf->m_aUiElements[i].m_Rect.x = lua_tonumber(L, 1);
    pSelf->m_aUiElements[i].m_Rect.y = lua_tonumber(L, 2);
    pSelf->m_aUiElements[i].m_Rect.w = lua_tonumber(L, 3);
    pSelf->m_aUiElements[i].m_Rect.h = lua_tonumber(L, 4);
    pSelf->m_aUiElements[i].m_RegPoint = lua_tonumber(L, 5);

    if (lua_isstring(L, 6))
        str_copy(pSelf->m_aUiElements[i].m_pText, lua_tostring(L, 6), sizeof(pSelf->m_aUiElements[i].m_pText));
    else
        pSelf->m_aUiElements[i].m_pText[0] = 0;

    if (lua_isnumber(L, 7))
        pSelf->m_aUiElements[i].m_FontSize = lua_tonumber(L, 7);
    else
        pSelf->m_aUiElements[i].m_FontSize = 14.0f;

    if (lua_isnumber(L, 8))
        pSelf->m_aUiElements[i].m_Align = lua_tonumber(L, 8);
    else
        pSelf->m_aUiElements[i].m_Align = 0;

    if (lua_isnumber(L, 9) && lua_isnumber(L, 10) && lua_isnumber(L, 11) && lua_isnumber(L, 12))
        pSelf->m_aUiElements[i].m_Color = vec4(lua_tonumber(L, 9), lua_tonumber(L, 10), lua_tonumber(L, 11), lua_tonumber(L, 12));
    else
        pSelf->m_aUiElements[i].m_Color = vec4(1, 1, 1, 1);

    pSelf->m_aUiElements[i].m_pClient = pSelf->m_pClient;
    pSelf->m_aUiElements[i].m_pLuaFile = pSelf;
    pSelf->m_aUiElements[i].m_Type = CLuaUi::LUAUILABEL;

    lua_pushinteger(L, i);

    return 1;
}

int CLuaFile::UiDoRect(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1))
        return 0;
    if (!lua_isnumber(L, 2))
        return 0;
    if (!lua_isnumber(L, 3))
        return 0;
    if (!lua_isnumber(L, 4))
        return 0;
    if (!lua_isnumber(L, 5))
        return 0;


    int i = 0;
    for (i = 0; i < LUAMAXUIELEMENTS; i++)
    {
        if (pSelf->m_aUiElements[i].m_Used == 0)
        {
            break;
        }
    }
    if (i >= LUAMAXUIELEMENTS)
        return 0;

    pSelf->m_aUiElements[i].m_Used = true;
    pSelf->m_aUiElements[i].m_Rect.x = lua_tonumber(L, 1);
    pSelf->m_aUiElements[i].m_Rect.y = lua_tonumber(L, 2);
    pSelf->m_aUiElements[i].m_Rect.w = lua_tonumber(L, 3);
    pSelf->m_aUiElements[i].m_Rect.h = lua_tonumber(L, 4);
    pSelf->m_aUiElements[i].m_RegPoint = lua_tonumber(L, 5);

    if (lua_isnumber(L, 6))
        pSelf->m_aUiElements[i].m_Corners = lua_tonumber(L, 6);
    else
        pSelf->m_aUiElements[i].m_Corners = CUI::CORNER_ALL;

    if (lua_isnumber(L, 7))
        pSelf->m_aUiElements[i].m_Rounding = lua_tonumber(L, 7);
    else
        pSelf->m_aUiElements[i].m_Rounding = 5.0f;

    if (lua_isnumber(L, 8) && lua_isnumber(L, 9) && lua_isnumber(L, 10) && lua_isnumber(L, 11))
        pSelf->m_aUiElements[i].m_Color = vec4(lua_tonumber(L, 8), lua_tonumber(L, 9), lua_tonumber(L, 10), lua_tonumber(L, 11));
    else
        pSelf->m_aUiElements[i].m_Color = vec4(0.0f, 0.0f, 0.0f, 0.5f);

    pSelf->m_aUiElements[i].m_pClient = pSelf->m_pClient;
    pSelf->m_aUiElements[i].m_pLuaFile = pSelf;
    pSelf->m_aUiElements[i].m_Type = CLuaUi::LUAUIRECT;

    lua_pushinteger(L, i);

    return 1;
}

int CLuaFile::UiDoImage(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1))
        return 0;
    if (!lua_isnumber(L, 2))
        return 0;
    if (!lua_isnumber(L, 3))
        return 0;
    if (!lua_isnumber(L, 4))
        return 0;
    if (!lua_isnumber(L, 5))
        return 0;
    if (!lua_isnumber(L, 6))
        return 0;
    if (!lua_isnumber(L, 7))
        return 0;
    if (!lua_isstring(L, 8))
        return 0;



    int i = 0;
    for (i = 0; i < LUAMAXUIELEMENTS; i++)
    {
        if (pSelf->m_aUiElements[i].m_Used == 0)
        {
            break;
        }
    }
    if (i >= LUAMAXUIELEMENTS)
        return 0;

    pSelf->m_aUiElements[i].m_Used = true;
    pSelf->m_aUiElements[i].m_Rect.x = lua_tonumber(L, 1);
    pSelf->m_aUiElements[i].m_Rect.y = lua_tonumber(L, 2);
    pSelf->m_aUiElements[i].m_Rect.w = lua_tonumber(L, 3);
    pSelf->m_aUiElements[i].m_Rect.h = lua_tonumber(L, 4);
    pSelf->m_aUiElements[i].m_RegPoint = lua_tonumber(L, 5);

    pSelf->m_aUiElements[i].m_TextureID = lua_tonumber(L, 6);
    pSelf->m_aUiElements[i].m_SpriteID = lua_tonumber(L, 7);
    str_copy(pSelf->m_aUiElements[i].m_pCallback, lua_tostring(L, 8), sizeof(pSelf->m_aUiElements[i].m_pCallback));

    pSelf->m_aUiElements[i].m_pClient = pSelf->m_pClient;
    pSelf->m_aUiElements[i].m_pLuaFile = pSelf;
    pSelf->m_aUiElements[i].m_Type = CLuaUi::LUAUIIMAGE;

    lua_pushinteger(L, i);

    return 1;
}

int CLuaFile::UiDoImageEx(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1))
        return 0;
    if (!lua_isnumber(L, 2))
        return 0;
    // 3 to 4 optional
    if (!lua_isnumber(L, 5))
        return 0;
    if (!lua_isnumber(L, 6))
        return 0;
    // 7 to 10 optional (clipping)
    if (!lua_isstring(L, 11))
        return 0;



    int i = 0;
    for (i = 0; i < LUAMAXUIELEMENTS; i++)
    {
        if (pSelf->m_aUiElements[i].m_Used == 0)
        {
            break;
        }
    }
    if (i >= LUAMAXUIELEMENTS)
        return 0;

    pSelf->m_aUiElements[i].m_Used = true;
    pSelf->m_aUiElements[i].m_Rect.x = lua_tonumber(L, 1);
    pSelf->m_aUiElements[i].m_Rect.y = lua_tonumber(L, 2);
    if (lua_isnumber(L, 3))
        pSelf->m_aUiElements[i].m_Rect.w = lua_tonumber(L, 3);
    else
        pSelf->m_aUiElements[i].m_Rect.w = pSelf->m_pClient->Graphics()->GetTextureWidth(lua_tointeger(L, 6));
    if (lua_isnumber(L, 4))
        pSelf->m_aUiElements[i].m_Rect.h = lua_tonumber(L, 4);
    else
        pSelf->m_aUiElements[i].m_Rect.h = pSelf->m_pClient->Graphics()->GetTextureHeight(lua_tointeger(L, 6));
    pSelf->m_aUiElements[i].m_RegPoint = lua_tonumber(L, 5);

    pSelf->m_aUiElements[i].m_TextureID = lua_tonumber(L, 6);

    int ImgWidth = pSelf->m_pClient->Graphics()->GetTextureWidth(lua_tointeger(L, 6));
    int ImgHeight = pSelf->m_pClient->Graphics()->GetTextureHeight(lua_tointeger(L, 6));
    float ClipX1 = 0.0f;
    float ClipY1 = 0.0f;
    float ClipX2 = 1.0f;
    float ClipY2 = 1.0f;
    if (lua_isnumber(L, 7))
        ClipX1 = clamp((float)lua_tointeger(L, 7) / (float)ImgWidth, 0.0f, 1.0f);
    if (lua_isnumber(L, 8))
        ClipY1 = clamp((float)lua_tointeger(L, 8) / (float)ImgHeight, 0.0f, 1.0f);
    if (lua_isnumber(L, 9))
        ClipX2 = clamp((float)lua_tointeger(L, 9) / (float)ImgWidth, 0.0f, 1.0f);
    if (lua_isnumber(L, 10))
        ClipY2 = clamp((float)lua_tointeger(L, 10) / (float)ImgHeight, 0.0f, 1.0f);

    pSelf->m_aUiElements[i].m_ClipX1 = ClipX1;
    pSelf->m_aUiElements[i].m_ClipY1 = ClipY1;
    pSelf->m_aUiElements[i].m_ClipX2 = ClipX2;
    pSelf->m_aUiElements[i].m_ClipY2 = ClipY2;
    str_copy(pSelf->m_aUiElements[i].m_pCallback, lua_tostring(L, 11), sizeof(pSelf->m_aUiElements[i].m_pCallback));

    pSelf->m_aUiElements[i].m_pClient = pSelf->m_pClient;
    pSelf->m_aUiElements[i].m_pLuaFile = pSelf;
    pSelf->m_aUiElements[i].m_Type = CLuaUi::LUAUIIMAGEEX;

    lua_pushinteger(L, i);

    return 1;
}

int CLuaFile::UiDoLine(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1)) //x1
        return 0;
    if (!lua_isnumber(L, 2)) //y1
        return 0;
    if (!lua_isnumber(L, 3)) //x2
        return 0;
    if (!lua_isnumber(L, 4)) //y2
        return 0;
    if (!lua_isnumber(L, 5))
        return 0;
    if (!lua_isnumber(L, 6))
        return 0;
    if (!lua_isnumber(L, 7))
        return 0;
    if (!lua_isnumber(L, 8))
        return 0;



    int i = 0;
    for (i = 0; i < LUAMAXUIELEMENTS; i++)
    {
        if (pSelf->m_aUiElements[i].m_Used == 0)
        {
            break;
        }
    }
    if (i >= LUAMAXUIELEMENTS)
        return 0;

    pSelf->m_aUiElements[i].m_Used = true;
    pSelf->m_aUiElements[i].m_Rect.x = lua_tonumber(L, 1);
    pSelf->m_aUiElements[i].m_Rect.y = lua_tonumber(L, 2);
    pSelf->m_aUiElements[i].m_Rect.w = lua_tonumber(L, 3);
    pSelf->m_aUiElements[i].m_Rect.h = lua_tonumber(L, 4);

    pSelf->m_aUiElements[i].m_Color = vec4(lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7), lua_tonumber(L, 8));

    pSelf->m_aUiElements[i].m_pClient = pSelf->m_pClient;
    pSelf->m_aUiElements[i].m_pLuaFile = pSelf;
    pSelf->m_aUiElements[i].m_Type = CLuaUi::LUAUILINE;

    lua_pushinteger(L, i);

    return 1;
}

int CLuaFile::UiDoSlider(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1))
        return 0;
    if (!lua_isnumber(L, 2))
        return 0;
    if (!lua_isnumber(L, 3))
        return 0;
    if (!lua_isnumber(L, 4))
        return 0;
    if (!lua_isnumber(L, 5))
        return 0;
    if (!lua_isnumber(L, 6))
        return 0;
    if (!lua_isstring(L, 7))
        return 0;


    int i = 0;
    for (i = 0; i < LUAMAXUIELEMENTS; i++)
    {
        if (pSelf->m_aUiElements[i].m_Used == 0)
        {
            break;
        }
    }
    if (i >= LUAMAXUIELEMENTS)
        return 0;

    pSelf->m_aUiElements[i].m_Used = true;
    pSelf->m_aUiElements[i].m_Rect.x = lua_tonumber(L, 1);
    pSelf->m_aUiElements[i].m_Rect.y = lua_tonumber(L, 2);
    pSelf->m_aUiElements[i].m_Rect.w = lua_tonumber(L, 3);
    pSelf->m_aUiElements[i].m_Rect.h = lua_tonumber(L, 4);
    pSelf->m_aUiElements[i].m_RegPoint = lua_tonumber(L, 5);

    pSelf->m_aUiElements[i].m_Value = lua_tonumber(L, 6);
    str_copy(pSelf->m_aUiElements[i].m_pCallback, lua_tostring(L, 7), sizeof(pSelf->m_aUiElements[i].m_pCallback));

    if (lua_isnumber(L, 8) && lua_isnumber(L, 9) && lua_isnumber(L, 10) && lua_isnumber(L, 11))
        pSelf->m_aUiElements[i].m_Color = vec4(lua_tonumber(L, 8), lua_tonumber(L, 9), lua_tonumber(L, 10), lua_tonumber(L, 11));
    else
        pSelf->m_aUiElements[i].m_Color = vec4(1,1,1,0.25f);

    if (lua_isnumber(L, 12))
        pSelf->m_aUiElements[i].m_Direction = lua_tonumber(L, 12);
    else
        pSelf->m_aUiElements[i].m_Direction = 0;

    pSelf->m_aUiElements[i].m_pClient = pSelf->m_pClient;
    pSelf->m_aUiElements[i].m_pLuaFile = pSelf;
    pSelf->m_aUiElements[i].m_Type = CLuaUi::LUAUISLIDER;

    lua_pushinteger(L, i);

    return 1;
}

int CLuaFile::UiDirectRect(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4))
        return 0;

    CUIRect Rect;
    vec4 Color = vec4(0, 0, 0, 0.5f);
    int Corners = CUI::CORNER_ALL;
    int Rounding = 5.0f;

    Rect.x = lua_tonumber(L, 1);
    Rect.y = lua_tonumber(L, 2);
    Rect.w = lua_tonumber(L, 3);
    Rect.h = lua_tonumber(L, 4);

    if (lua_isnumber(L, 5) && lua_isnumber(L, 6) && lua_isnumber(L, 7) && lua_isnumber(L, 8))
        Color = vec4(lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7), lua_tonumber(L, 8));

    if (lua_isnumber(L, 9))
        Corners = lua_tointeger(L, 9);

    if (lua_isnumber(L, 10))
        Rounding = lua_tonumber(L, 10);

    pSelf->m_pClient->RenderTools()->DrawUIRect(&Rect, Color, Corners, Rounding);

    return 0;
}

int CLuaFile::BlendNormal(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    pSelf->m_pClient->Graphics()->BlendNormal();

    return 0;
}

int CLuaFile::BlendAdditive(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    pSelf->m_pClient->Graphics()->BlendAdditive();

    return 0;
}

int CLuaFile::UiGetGameTextureID(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushinteger(L, g_pData->m_aImages[IMAGE_GAME].m_Id);
    return 1;
}

int CLuaFile::UiGetParticleTextureID(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushinteger(L, g_pData->m_aImages[IMAGE_PARTICLES].m_Id);
    return 1;
}

int CLuaFile::UiGetFlagTextureID(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1))
        return 0;

    lua_pushinteger(L, pSelf->m_pClient->m_pCountryFlags->GetByCountryCode(lua_tonumber(L, 1))->m_Texture);
    return 1;
}

int CLuaFile::TextureLoad(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isstring(L, 1))
        return 0;

    int ID = pSelf->m_pClient->Graphics()->LoadTexture(lua_tostring(L, 1), IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, IGraphics::TEXLOAD_NORESAMPLE);
    pSelf->m_lTextures.add(ID);

    lua_pushinteger(L, ID);
    return 1;
}

int CLuaFile::TextureUnload(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1))
        return 0;

    pSelf->m_pClient->Graphics()->UnloadTexture(lua_tointeger(L, 1));
    return 0;
}

int CLuaFile::ReplaceGameTexture(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (lua_isnumber(L, 1))
        g_pData->m_aImages[IMAGE_GAME].m_Id = lua_tointeger(L, 1);
    return 0;
}

int CLuaFile::RenderTexture(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    //1 texture
    //2 x
    //3 y
    //4 (width)
    //5 (height)
    //6 (clip_x1)
    //7 (clip_y1)
    //8 (clip_x2)
    //9 (clip_y2)
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
        return 0;

    float x = lua_tonumber(L, 2);
    float y = lua_tonumber(L, 3);
    int ImgWidth = pSelf->m_pClient->Graphics()->GetTextureWidth(lua_tointeger(L, 1));
    int ImgHeight = pSelf->m_pClient->Graphics()->GetTextureHeight(lua_tointeger(L, 1));
    float ClipX1 = 0.0f;
    float ClipY1 = 0.0f;
    float ClipX2 = 1.0f;
    float ClipY2 = 1.0f;
    if (lua_isnumber(L, 6))
        ClipX1 = clamp((float)lua_tonumber(L, 6) / (float)ImgWidth, 0.0f, 1.0f);
    if (lua_isnumber(L, 7))
        ClipY1 = clamp((float)lua_tonumber(L, 7) / (float)ImgHeight, 0.0f, 1.0f);
    if (lua_isnumber(L, 8))
        ClipX2 = clamp((float)lua_tonumber(L, 8) / (float)ImgWidth, 0.0f, 1.0f);
    if (lua_isnumber(L, 9))
        ClipY2 = clamp((float)lua_tonumber(L, 9) / (float)ImgHeight, 0.0f, 1.0f);


    float Width = abs(ClipX2 - ClipX1) * ImgWidth;
    float Height = abs(ClipY2 - ClipY1) * ImgHeight;
    if (lua_isnumber(L, 4))
        Width = lua_tonumber(L, 4);
    if (lua_isnumber(L, 5))
        Height = lua_tonumber(L, 5);

    if (pSelf->m_pClient->Graphics()->OnScreen(x, y, Width, Height) == false)
        return 0;

    pSelf->m_pClient->Graphics()->TextureSet(lua_tointeger(L, 1));
    pSelf->m_pClient->Graphics()->QuadsBegin();
    if (lua_isnumber(L, 10) && lua_isnumber(L, 11) && lua_isnumber(L, 12) && lua_isnumber(L, 13))
        pSelf->m_pClient->Graphics()->SetColor(lua_tonumber(L, 10), lua_tonumber(L, 11), lua_tonumber(L, 12), lua_tonumber(L, 13));
    if (lua_isnumber(L, 14))
        pSelf->m_pClient->Graphics()->QuadsSetRotation(lua_tonumber(L, 14));
    pSelf->m_pClient->Graphics()->QuadsSetSubset(ClipX1, ClipY1, ClipX2, ClipY2);
    IGraphics::CQuadItem QuadItem(x, y, Width, Height);
    pSelf->m_pClient->Graphics()->QuadsDrawTL(&QuadItem, 1);
    pSelf->m_pClient->Graphics()->QuadsEnd();
    return 0;
}

int CLuaFile::LoadWvFile(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if(!lua_isstring(L, 1))
        return 0;

    int ID = pSelf->m_pClient->Sound()->LoadWV(lua_tostring(L, 1));
    pSelf->m_lSounds.add(ID);
    lua_pushinteger(L, ID);
    return 1;
}

int CLuaFile::PlayWv(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if(!lua_isnumber(L, 1))
        return 0;

    int Flags = ISound::FLAG_POS;
    if (lua_isnumber(L, 2) && lua_tointeger(L, 2) == 1)
        Flags = 0;

    float x = 0;
    float y = 0;

    if (lua_isnumber(L, 3) && lua_isnumber(L, 4))
    {
        x = lua_tonumber(L, 3);
        y = lua_tonumber(L, 4);
    }

    pSelf->m_pClient->Sound()->PlayAt(CSounds::CHN_WORLD, lua_tointeger(L, 1), Flags, x, y);
    return 0;
}

int CLuaFile::PlaySound(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if(!lua_isnumber(L, 1))
        return 0;


    float x = 0;
    float y = 0;

    if (lua_isnumber(L, 2) && lua_isnumber(L, 3))
    {
        x = lua_tonumber(L, 2);
        y = lua_tonumber(L, 3);
    }

    dbg_msg("", "play sound: %i", lua_tointeger(L, 1));

    pSelf->m_pClient->m_pSounds->Play(CSounds::CHN_WORLD, lua_tointeger(L, 1), 1.0f, vec2(x, y));
    return 0;
}

int CLuaFile::SendPacket(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if(lua_isnil(L, 1))
        return 0;

	char aData[2000];
	int Size = sizeof(aData);
	CMsgPacker P(NETMSG_LUA_DATA);
	if (compress2((Bytef *)aData, (uLongf *)&Size, (Bytef *)lua_tostring(L, 1), str_length(lua_tostring(L, 1)), Z_BEST_COMPRESSION) == Z_OK && Size < str_length(lua_tostring(L, 1)))
	{
        P.AddInt(Size);
        P.AddRaw(aData, Size);
	}
	else
	{
	    str_copy(aData, lua_tostring(L, 1), sizeof(aData));
	    P.AddInt(-1); //no compression
	    P.AddString(aData, sizeof(aData));
	}
    pSelf->m_pClient->Client()->SendMsgEx(&P, MSGFLAG_VITAL|MSGFLAG_FLUSH, true);

    return 0;
}

int CLuaFile::FetchPacket(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!pSelf->m_pLuaHandler->m_EventListener.m_pNetData)
        return 0;
	lua_pushstring(L, pSelf->m_pLuaHandler->m_EventListener.m_pNetData);
    return 1;
}



int CLuaFile::GetNumGroups(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

	if(!pSelf->m_pClient->Layers())
	{
		lua_pushnumber(L, -1);
		return 1;
	}

    lua_pushnumber(L, pSelf->m_pClient->Layers()->NumGroups());
    return 1;
}
int CLuaFile::GetNumLayers(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

	if(!pSelf->m_pClient->Layers())
	{
		lua_pushnumber(L, -1);
		return 1;
	}

    lua_pushnumber(L, pSelf->m_pClient->Layers()->NumLayers());
    return 1;
}
int CLuaFile::GetGroupNumLayers(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

	if(!lua_isnumber(L, 1) || !pSelf->m_pClient->Layers() || pSelf->m_pClient->Layers()->NumGroups() < 1)
	{
		lua_pushnumber(L, -1);
		return 1;
	}
	if(!pSelf->m_pClient->Layers()->GetGroup(lua_tointeger(L, 1)))
	{
		lua_pushnumber(L, -1);
		return 1;
	}
	lua_pushnumber(L, pSelf->m_pClient->Layers()->GetGroup(lua_tointeger(L, 1))->m_NumLayers);
    return 1;
}

int CLuaFile::GetLayerType(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

	if(!lua_isnumber(L, 1) || !pSelf->m_pClient->Layers() || pSelf->m_pClient->Layers()->NumGroups() < 1)
	{
		lua_pushnumber(L, -1);
		return 1;
	}
	int Group = 0;
	int Index = lua_tointeger(L, 1);
	if(lua_isnumber(L, 2))
	{
		Group = clamp((int)lua_tointeger(L, 1), 0, pSelf->m_pClient->Layers()->NumGroups()-1);
		if(pSelf->m_pClient->Layers()->GetGroup(Group))
			Index = clamp((int)lua_tointeger(L, 2), 0, pSelf->m_pClient->Layers()->GetGroup(Group)->m_NumLayers-1);
		else
		{
			lua_pushnumber(L, -1);
			return 1;
		}
	}
	else
		Index = clamp(Index, 0, pSelf->m_pClient->Layers()->NumLayers());
	if(!pSelf->m_pClient->Layers()->GetGroup(Group))
	{
			lua_pushnumber(L, -1);
			return 1;
	}
    lua_pushnumber(L, pSelf->m_pClient->Layers()->GetLayer(pSelf->m_pClient->Layers()->GetGroup(Group)->m_StartLayer+Index)->m_Type);
    return 1;
}
int CLuaFile::GetLayerFlags(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

	if(!lua_isnumber(L, 1) || !pSelf->m_pClient->Layers() || pSelf->m_pClient->Layers()->NumGroups() < 1)
	{
		lua_pushnumber(L, -1);
		return 1;
	}
	int Group = 0;
	int Index = lua_tointeger(L, 1);
	if(lua_isnumber(L, 2))
	{
		Group = clamp((int)lua_tointeger(L, 1), 0, pSelf->m_pClient->Layers()->NumGroups()-1);
		if(pSelf->m_pClient->Layers()->GetGroup(Group))
			Index = clamp((int)lua_tointeger(L, 2), 0, pSelf->m_pClient->Layers()->GetGroup(Group)->m_NumLayers-1);
		else
		{
			lua_pushnumber(L, -1);
			return 1;
		}
	}
	else
		Index = clamp(Index, 0, pSelf->m_pClient->Layers()->NumLayers());

	if(!pSelf->m_pClient->Layers()->GetGroup(Group))
	{
		lua_pushnumber(L, -1);
		lua_pushnumber(L, -1);
		return 2;
	}

	CMapItemLayerTilemap *pTmap = (CMapItemLayerTilemap *)pSelf->m_pClient->Layers()->GetLayer(pSelf->m_pClient->Layers()->GetGroup(Group)->m_StartLayer+Index);
    lua_pushnumber(L, pTmap->m_Flags);
    return 1;
}
int CLuaFile::GetLayerSize(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

	if(!lua_isnumber(L, 1) || !pSelf->m_pClient->Layers() || pSelf->m_pClient->Layers()->NumGroups() < 1)
	{
		lua_pushnumber(L, -1);
		return 1;
	}
	int Group = 0;
	int Index = lua_tointeger(L, 1);
	if(lua_isnumber(L, 2))
	{
		Group = clamp((int)lua_tointeger(L, 1), 0, pSelf->m_pClient->Layers()->NumGroups()-1);
		if(pSelf->m_pClient->Layers()->GetGroup(Group))
			Index = clamp((int)lua_tointeger(L, 2), 0, pSelf->m_pClient->Layers()->GetGroup(Group)->m_NumLayers-1);
		else
		{
			lua_pushnumber(L, -1);
			lua_pushnumber(L, -1);
			return 2;
		}
	}
	else
		Index = clamp(Index, 0, pSelf->m_pClient->Layers()->NumLayers());

	if(!pSelf->m_pClient->Layers()->GetGroup(Group))
	{
		lua_pushnumber(L, -1);
		lua_pushnumber(L, -1);
		return 2;
	}
	CMapItemLayerTilemap *pTmap = (CMapItemLayerTilemap *)pSelf->m_pClient->Layers()->GetLayer(pSelf->m_pClient->Layers()->GetGroup(Group)->m_StartLayer+Index);

	if(!pTmap)
	{
		lua_pushnumber(L, -1);
		lua_pushnumber(L, -1);
		return 2;
	}

    lua_pushnumber(L, pTmap->m_Width);
    lua_pushnumber(L, pTmap->m_Height);
    return 2;
}

int CLuaFile::GetLayerTileFlags(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

	if(!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)  || !pSelf->m_pClient->Layers()  || pSelf->m_pClient->Layers()->NumGroups() < 1)
	{
		lua_pushnumber(L, -1);
		return 1;
	}
	int Group = 0;
	int Index = lua_tointeger(L, 1);
	int x = lua_tointeger(L, 2);
	int y = lua_tointeger(L, 3);
	if(lua_isnumber(L, 4))
	{
		Group = clamp((int)lua_tointeger(L, 1), 0, pSelf->m_pClient->Layers()->NumGroups()-1);
		if(pSelf->m_pClient->Layers()->GetGroup(Group))
			Index = clamp((int)lua_tointeger(L, 2), 0, pSelf->m_pClient->Layers()->GetGroup(Group)->m_NumLayers-1);
		else
		{
			lua_pushnumber(L, -1);
			return 1;
		}
		x = lua_tointeger(L, 3);
		y = lua_tointeger(L, 4);
	}
	else
		Index = clamp(Index, 0, pSelf->m_pClient->Layers()->NumLayers());
	if(!pSelf->m_pClient->Layers()->GetGroup(Group))
	{
		lua_pushnumber(L, -1);
		return 1;
	}
	CMapItemLayer *pLayer = pSelf->m_pClient->Layers()->GetLayer(pSelf->m_pClient->Layers()->GetGroup(Group)->m_StartLayer+Index);
	if(!pLayer)
	{
		lua_pushnumber(L, -1);
		return 1;
	}
	if(pLayer->m_Type != LAYERTYPE_TILES)
    {
		lua_pushnumber(L, -1);
		return 1;
	}
	CMapItemLayerTilemap *pTmap = (CMapItemLayerTilemap *)pLayer;
    CTile *pTiles = (CTile *)pSelf->m_pClient->Layers()->Map()->GetData(pTmap->m_Data);


    lua_pushnumber(L, pTiles[y*pTmap->m_Width+x].m_Flags);
    return 1;
}

int CLuaFile::SetLayerTileFlags(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

	if(!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4) || !pSelf->m_pClient->Layers()  || pSelf->m_pClient->Layers()->NumGroups() < 1)
		return 0;

	int Group = 0;
	int Index = lua_tointeger(L, 1);
	int x = lua_tointeger(L, 2);
	int y = lua_tointeger(L, 3);
	int NewFlags = lua_tointeger(L, 4);
	if(lua_isnumber(L, 5))
	{
		Group = clamp((int)lua_tointeger(L, 1), 0, pSelf->m_pClient->Layers()->NumGroups()-1);
		if(pSelf->m_pClient->Layers()->GetGroup(Group))
			Index = clamp((int)lua_tointeger(L, 2), 0, pSelf->m_pClient->Layers()->GetGroup(Group)->m_NumLayers-1);
		else
			return 0;

		x = lua_tointeger(L, 3);
		y = lua_tointeger(L, 4);
		NewFlags = lua_tointeger(L, 5);
	}
	else
		Index = clamp(Index, 0, pSelf->m_pClient->Layers()->NumLayers());
	if(!pSelf->m_pClient->Layers()->GetGroup(Group))
		return 0;
	CMapItemLayer *pLayer = pSelf->m_pClient->Layers()->GetLayer(pSelf->m_pClient->Layers()->GetGroup(Group)->m_StartLayer+Index);
	if(!pLayer)
		return 0;
	if(pLayer->m_Type != LAYERTYPE_TILES)
		return 0;

	CMapItemLayerTilemap *pTmap = (CMapItemLayerTilemap *)pLayer;
    CTile *pTiles = (CTile *)pSelf->m_pClient->Layers()->Map()->GetData(pTmap->m_Data);


	pTiles[y*pTmap->m_Width+x].m_Flags = NewFlags;
    return 0;
}
int CLuaFile::GetLayerTileIndex(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

	if(!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)  || !pSelf->m_pClient->Layers()  || pSelf->m_pClient->Layers()->NumGroups() < 1)
	{
		lua_pushnumber(L, -1);
		return 1;
	}
	int Group = 0;
	int Index = lua_tointeger(L, 1);
	int x = lua_tointeger(L, 2);
	int y = lua_tointeger(L, 3);
	if(lua_isnumber(L, 4))
	{
		Group = clamp((int)lua_tointeger(L, 1), 0, pSelf->m_pClient->Layers()->NumGroups()-1);
		if(pSelf->m_pClient->Layers()->GetGroup(Group))
			Index = clamp((int)lua_tointeger(L, 2), 0, pSelf->m_pClient->Layers()->GetGroup(Group)->m_NumLayers-1);
		else
		{
			lua_pushnumber(L, -1);
			return 1;
		}
		x = lua_tointeger(L, 3);
		y = lua_tointeger(L, 4);
	}
	else
		Index = clamp(Index, 0, pSelf->m_pClient->Layers()->NumLayers());

	if(!pSelf->m_pClient->Layers()->GetGroup(Group))
	{
		lua_pushnumber(L, -1);
		return 1;
	}

	CMapItemLayer *pLayer = pSelf->m_pClient->Layers()->GetLayer(pSelf->m_pClient->Layers()->GetGroup(Group)->m_StartLayer+Index);
	if(!pLayer)
	{
		lua_pushnumber(L, -1);
		return 1;
	}
	if(pLayer->m_Type != LAYERTYPE_TILES)
    {
		lua_pushnumber(L, -1);
		return 1;
	}
	CMapItemLayerTilemap *pTmap = (CMapItemLayerTilemap *)pLayer;
    CTile *pTiles = (CTile *)pSelf->m_pClient->Layers()->Map()->GetData(pTmap->m_Data);


    lua_pushnumber(L, pTiles[y*pTmap->m_Width+x].m_Index);
    return 1;
}

int CLuaFile::SetLayerTileIndex(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

	if(!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4) || !pSelf->m_pClient->Layers()  || pSelf->m_pClient->Layers()->NumGroups() < 1)
		return 0;

	int Group = 0;
	int Index = lua_tointeger(L, 1);
	int x = lua_tointeger(L, 2);
	int y = lua_tointeger(L, 3);
	int NewIndex = lua_tointeger(L, 4);
	if(lua_isnumber(L, 5))
	{
		Group = clamp((int)lua_tointeger(L, 1), 0, pSelf->m_pClient->Layers()->NumGroups()-1);
		if(pSelf->m_pClient->Layers()->GetGroup(Group))
			Index = clamp((int)lua_tointeger(L, 2), 0, pSelf->m_pClient->Layers()->GetGroup(Group)->m_NumLayers-1);
		else
			return 0;
		x = lua_tointeger(L, 3);
		y = lua_tointeger(L, 4);
		NewIndex = lua_tointeger(L, 5);
	}
	else
		Index = clamp((int)Index, 0, pSelf->m_pClient->Layers()->NumLayers());

	if(!pSelf->m_pClient->Layers()->GetGroup(Group))
			return 0;
	CMapItemLayer *pLayer = pSelf->m_pClient->Layers()->GetLayer(pSelf->m_pClient->Layers()->GetGroup(Group)->m_StartLayer+Index);
	if(!pLayer)
		return 0;
	if(pLayer->m_Type != LAYERTYPE_TILES)
		return 0;

	CMapItemLayerTilemap *pTmap = (CMapItemLayerTilemap *)pLayer;
    CTile *pTiles = (CTile *)pSelf->m_pClient->Layers()->Map()->GetData(pTmap->m_Data);


	pTiles[y*pTmap->m_Width+x].m_Index = NewIndex;
    return 0;
}

int CLuaFile::RenderTilemapGenerateSkip(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

	if(pSelf->m_pClient->Layers() && pSelf->m_pClient->RenderTools())
		pSelf->m_pClient->RenderTools()->RenderTilemapGenerateSkip(pSelf->m_pClient->Layers());
    return 0;
}

int CLuaFile::GetKeyFlags(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushinteger(L, pSelf->m_pLuaHandler->m_EventListener.m_KeyEvent.m_Flags);
    return 1;
}

int CLuaFile::GetKeyCode(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushinteger(L, pSelf->m_pLuaHandler->m_EventListener.m_KeyEvent.m_Key);
    return 1;
}

int CLuaFile::GetKeyUnicode(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushinteger(L, pSelf->m_pLuaHandler->m_EventListener.m_KeyEvent.m_Unicode);
    return 1;
}

int CLuaFile::SetLocalCharacterPos(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    pSelf->m_pClient->m_LocalCharacterPos = vec2(lua_tonumber(L, 1), lua_tonumber(L, 2));
    return 1;
}


int CLuaFile::DemoStart(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isstring(L, 1))
        return 0;

    pSelf->m_pClient->Client()->DemoRecorder_Stop();
    pSelf->m_pClient->Client()->DemoRecorder_Start(lua_tostring(L, 1), false);
    return 0;
}

int CLuaFile::DemoStop(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    pSelf->m_pClient->Client()->DemoRecorder_Stop();
    return 0;
}

int CLuaFile::DemoDelete(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isstring(L, 1))
        return 0;

    char aBuf[1024];
    str_format(aBuf, sizeof(aBuf), "demos/%s", lua_tostring(L, 1));

    pSelf->m_pClient->Storage()->RemoveFile(aBuf, IStorage::TYPE_SAVE);
    return 0;
}

int CLuaFile::StatGetNumber(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    IOHANDLE IndexFile = pSelf->m_pClient->Storage()->OpenFile("stats/index.stat", IOFLAG_READ, IStorage::TYPE_ALL);
    if (IndexFile)
    {
        CStats::CStatsIndexRow Row;
        int Num = 0;
        while(io_read(IndexFile, &Row, sizeof(Row)))
        {
            Num++;
        }
        lua_pushinteger(L, Num);
        io_close(IndexFile);
    }
    else
        lua_pushinteger(L, 0);

    return 1;
}

int CLuaFile::StatGetInfo(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1))
        return 0;

    IOHANDLE IndexFile = pSelf->m_pClient->Storage()->OpenFile("stats/index.stat", IOFLAG_READ, IStorage::TYPE_ALL);
    if (IndexFile)
    {
        CStats::CStatsIndexRow Row;
        io_seek(IndexFile, lua_tointeger(L, 1) * sizeof(Row), IOSEEK_START);
        io_read(IndexFile, &Row, sizeof(Row));
        char aBuf[256];
        net_addr_str(&Row.m_ServerAddr, aBuf, sizeof(aBuf), 1);
        lua_pushstring(L, aBuf);
        lua_pushstring(L, Row.m_aMap);
        lua_pushstring(L, Row.m_aGameType);
        lua_pushstring(L, Row.m_aServerName);
        lua_pushinteger(L, Row.m_TimeStamp);
        lua_pushinteger(L, Row.m_Uid);

        io_close(IndexFile);
        return 6;
    }

    return 0;
}

int CLuaFile::StatGetRow(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    if (!lua_isnumber(L, 1) && !lua_isnumber(L, 2))
        return 0;
    char aBuf[256];
    str_format(aBuf, sizeof(aBuf), "stats/%i.stat", lua_tointeger(L, 1));
    IOHANDLE StatFile = pSelf->m_pClient->Storage()->OpenFile(aBuf, IOFLAG_READ, IStorage::TYPE_ALL);
    int Ret = 0;
    if (StatFile)
    {
        CStatsRecords::CRecordRow Row;
        io_seek(StatFile, lua_tointeger(L, 2) * sizeof(Row), IOSEEK_START);
        io_read(StatFile, &Row, sizeof(Row));
        lua_pushinteger(L, Row.m_aData[0]);
        if (Row.m_aData[0] == CStats::STATROW_SERVER)
        {
            lua_pushinteger(L, Row.m_aData[1]);
            Ret = 2;
        }
        if (Row.m_aData[0] == CStats::STATROW_KILL)
        {
            CNetMsg_Sv_KillMsg *pMsg = (CNetMsg_Sv_KillMsg *)&Row.m_aData[1];
            lua_pushinteger(L, pMsg->m_Killer);
            lua_pushinteger(L, pMsg->m_ModeSpecial);
            lua_pushinteger(L, pMsg->m_Victim);
            lua_pushinteger(L, pMsg->m_Weapon);
            Ret = 5;
        }

        io_close(StatFile);
    }

    return Ret;
}

int CLuaFile::TimeGet(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushnumber(L, (float)time_get() / (float)time_freq());
    return 1;
}

int CLuaFile::FPS(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLuaFile *pSelf = (CLuaFile *)lua_touserdata(L, -1);
    lua_Debug Frame;
    lua_getstack(L, 1, &Frame);
    lua_getinfo(L, "nlSf", &Frame);

    lua_pushnumber(L, 1.0f / pSelf->m_pClient->Client()->FrameTime());
    return 1;
}
