/* (c) MAP94 and Patafix. See www.n-lvl.com/ndc/nclient/ for more information. */
#ifndef GAME_LUA_SERVER_H
#define GAME_LUA_SERVER_H

#include <game/server/gamecontext.h>
#include <game/server/entities/projectile.h>
#include <engine/shared/config.h>
#include <engine/config.h>
#include <base/tl/sorted_array.h>
//#include <engine/lua.h>

extern "C" { // lua
    #define LUA_CORE /* make sure that we don't try to import these functions */
    #include <engine/external/lua/lua.h>
    #include <engine/external/lua/lualib.h> /* luaL_openlibs */
    #include <engine/external/lua/lauxlib.h> /* luaL_loadfile */
}

class CLuaEventListener
{
    struct CLuaListenerData
    {
        class CLuaFile *m_pLuaFile;
        char m_aLuaFunction[256];
        char m_aEvent[256];
        bool operator==(const CLuaListenerData &Other) { return this == &Other; }
    };

    array<CLuaListenerData> m_aListeners;
public:
    void AddEventListener(class CLuaFile *pLuaFile, char *pEvent, char *pLuaFunction);
    void RemoveEventListener(class CLuaFile *pLuaFile, char *pEvent);

    void OnEvent(char *pEvent);

    CLuaEventListener();
    ~CLuaEventListener();

    //Chat OnChat
    char *m_pChatText;
    int m_ChatClientID;
    int m_ChatTeam;
    bool m_ChatHide;

    //Kill
    int m_KillKillerID;
    int m_KillVictimID;
    int m_KillWeapon;

    //OnNetData
    char *m_pNetData;
    int m_pNetClientID;

	//OnWeaponFire
	int m_OnWeaponFireClientID;
	int m_OnWeaponFireWeaponID;
	vec2 m_OnWeaponFireDir;

	//Jump
	int m_OnJumpJumpID;
	int m_OnJumpClientID;
};

class CLuaFile
{
public:
    CLuaFile();
    ~CLuaFile();
    class CLua *m_pLuaHandler;
    CGameContext *m_pServer;
    //void UiTick();
    void Tick();
    void TickDefered();
    void PostTick();
    void End();
    void Close();
    void Init(const char *pFile);

    //Some Error and Lua stuff
    //Error
    static int ErrorFunc(lua_State *L);
    static int Panic(lua_State *L); //lua panic function
    lua_State *m_pLua;

    char *GetScriptName() {return m_aFilename;};

    //Settings
    void ConfigClose(); //Helper function
    bool m_HaveSettings;
    char m_aTitle[64];
    char m_aFilename[256];
    char m_aInfo[256];

    bool FunctionExist(const char *pFunctionName);
    void FunctionExec(const char *pFunctionName = 0);
    void FunctionPrepare(const char *pFunctionName);
    void PushString(const char *pString);
    void PushInteger(int value);
    void PushFloat(float value);
    void PushBoolean(bool value);
    void PushParameter(const char *pString);
    int m_FunctionVarNum;

    //Functions:
    //Settings
    static inline int SetScriptUseSettingPage(lua_State *L);
    static inline int SetScriptTitle(lua_State *L);
    static inline int SetScriptInfo(lua_State *L);

    //Eventlistener stuff
    static inline int AddEventListener(lua_State *L);
    static inline int RemoveEventListener(lua_State *L);

    //Menu Browser Things
    static inline int SetMenuBrowserGameTypeColor(lua_State *L);
    static inline int GetMenuBrowserGameTypeName(lua_State *L);

    //Chat
    static inline int ChatGetText(lua_State *L);
    static inline int ChatGetClientID(lua_State *L);
    static inline int ChatGetTeam(lua_State *L);
    static inline int ChatHide(lua_State *L);

    //Kill
    static inline int KillGetKillerID(lua_State *L);
    static inline int KillGetVictimID(lua_State *L);
    static inline int KillGetWeapon(lua_State *L);


	//WeaponFire
    static inline int WeaponFireGetClientID(lua_State *L);
    static inline int WeaponFireGetWeaponID(lua_State *L);
    static inline int WeaponFireGetDir(lua_State *L);

	//Jump
    static inline int JumpGetClientID(lua_State *L);
    static inline int JumpGetJumpID(lua_State *L);

    //
    //Include
    static inline int Include(lua_State *L);

    //emote
    static inline int Emote(lua_State *L);

    //Character
    static inline int GetCharacterPos(lua_State *L);
    static inline int SetCharacterPos(lua_State *L);
    static inline int GetCharacterVel(lua_State *L);
    static inline int SetCharacterVel(lua_State *L);

    //collision
    static inline int IntersectLine(lua_State *L);
    static inline int GetTile(lua_State *L);
    static inline int SetTile(lua_State *L);
    static inline int GetMapWidth(lua_State *L);
    static inline int GetMapHeight(lua_State *L);

	// static inline int CreateParticle(lua_State *L);

    //Console Print
    static inline int Print(lua_State *L);
    static inline int Console(lua_State *L);

    //Serverinfo
    static inline int GetGameType(lua_State *L);
    static inline int IsTeamplay(lua_State *L);

    //Chat
    static inline int SendChat(lua_State *L);
    static inline int SendChatTarget(lua_State *L);

    //Player  Todo: PlayerSet
    static inline int GetPlayerName(lua_State *L);
    static inline int GetPlayerClan(lua_State *L);
    static inline int GetPlayerCountry(lua_State *L);
    static inline int GetPlayerScore(lua_State *L);
    static inline int GetPlayerPing(lua_State *L);
    static inline int GetPlayerTeam(lua_State *L);
    static inline int GetPlayerSkin(lua_State *L);
    static inline int GetPlayerColorFeet(lua_State *L); //Todo: implement me
    static inline int GetPlayerColorBody(lua_State *L); //Todo: implement me
    static inline int GetPlayerColorSkin(lua_State *L); //Todo: implement me

    //Config
    static inline int GetConfigValue(lua_State *L);
    static inline int SetConfigValue(lua_State *L);

    //LuaNetWork
    static inline int FetchPacket(lua_State *L);
    static inline int GetPacketClientID(lua_State *L);
    static inline int SendPacket(lua_State *L);
    static inline int AddModFile(lua_State *L);
    static inline int DeleteModFile(lua_State *L);
    static inline int SendFile(lua_State *L);

    //Entities
    static inline int EntityFind(lua_State *L);
    static inline int EntityGetPos(lua_State *L);
    static inline int EntitySetPos(lua_State *L);
    static inline int EntityDestroy(lua_State *L);
    static inline int ProjectileGetWeapon(lua_State *L);
};

class CLua
{
public:
    CGameContext *m_pServer;
    CLua(CGameContext *pServer);
    ~CLua();
    void Tick();
	void TickDefered();
    void PostTick();
    bool Init(const char *pFile);
    void End();
    void Close();

    CLuaFile m_aLuaFiles[MAX_LUA_FILES];
    CLuaEventListener m_EventListener;

    //Search the file and execs the function
    void ConfigClose(char *pFilename);

    int GetFileId(char *pFilename);

};


//helper functions
static int StrIsInteger(const char *pStr)
{
	while(*pStr)
	{
		if(!(*pStr >= '0' && *pStr <= '9'))
			return 0;
		pStr++;
	}
	return 1;
}
static int StrIsFloat(const char *pStr)
{
	while(*pStr)
	{
		if(!(*pStr >= '0' && *pStr <= '9' || *pStr == '.'))
			return 0;
		pStr++;
	}
	return 1;
}

#endif
