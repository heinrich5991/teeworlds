/* (c) MAP94. See www.n-lvl.com/ndc/nclient/ for more information. */
#ifndef ENGINE_SERVER_LUA_H
#define ENGINE_SERVER_LUA_H

#include <engine/lua.h>

class CLua : public ILua
{
    static void ConfigSaveCallback(IConfig *pConfig, void *pUserData);
    static void ConListLuaFiles(IConsole::IResult *pResult, void *pUserData);
    static void ConDeleteAllLuaFiles(IConsole::IResult *pResult, void *pUserData);
    static void ConDeleteLuaFile(IConsole::IResult *pResult, void *pUserData);
	IConsole* m_pConsole;

public:
	CLua();

	char m_aLuaFiles[MAX_LUA_FILES][1024];
	bool m_aLuaFilesSave[MAX_LUA_FILES];
	char *GetFileDir(int i);
    void DeleteLuaFile(int i);
    void DeleteLuaFile(char *pFileDir);
    void DeleteAllLuaFiles();
    void AddLuaFile(char *pFileDir, bool NoSave = false);

	bool GetLuaSaveOption(int i){if(i >= 0 && i < MAX_LUA_FILES) {return m_aLuaFilesSave[i];} return false;};

	static void ConAddLuaFile(IConsole::IResult *pResult, void *pUserData);

	void Init();

	IConsole* Console() {return m_pConsole;};
};

#endif
