/* (c) MAP94. See www.n-lvl.com/ndc/nclient/ for more information. */
#ifndef ENGINE_CLIENT_LUA_H
#define ENGINE_CLIENT_LUA_H

#include <engine/lua.h>

class CLua : public ILua
{
	static void ConAddLuaFile(IConsole::IResult *pResult, void *pUserData);
    static void ConfigSaveCallback(IConfig *pConfig, void *pUserData);

public:
	CLua();

	char *m_aLuaFiles[MAX_LUA_FILES];
	char *GetFileDir(int i);
    void DeleteLuaFile(int i);
    void AddLuaFile(char *pFilename);

	void Init();
};

#endif
