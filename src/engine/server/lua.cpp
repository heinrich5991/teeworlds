/* (c) MAP94. See www.n-lvl.com/ndc/nclient/ for more information. */
#include <base/math.h>

#include <engine/config.h>
#include <engine/console.h>
#include <engine/shared/config.h>

#include "lua.h"

CLua::CLua()
{
	mem_zero(m_aLuaFiles, sizeof(m_aLuaFiles));
}

void CLua::ConAddLuaFile(IConsole::IResult *pResult, void *pUserData)
{
    ((CLua*)pUserData)->AddLuaFile((char *)pResult->GetString(0));
}

void CLua::AddLuaFile(char *pFileDir, bool NoSave)
{
	char aBuf[1024];
	str_format(aBuf, sizeof(aBuf), "lua/%s", pFileDir);
	int Free = -1;
    for (int i = 0; i < MAX_LUA_FILES; i++)
    {
		if (m_aLuaFiles[i] && !str_comp_nocase(m_aLuaFiles[i], aBuf))
			Free = -2;
        if (m_aLuaFiles[i][0] == 0 && str_comp(m_aLuaFiles[i], aBuf) && Free == -1) //lua inactiv
        	Free = i;
		if(Free == -2)
			break;
    }

    if (Free > -1 && Free < MAX_LUA_FILES)
		str_copy(m_aLuaFiles[Free], aBuf, sizeof(m_aLuaFiles[Free]));
}
void CLua::ConListLuaFiles(IConsole::IResult *pResult, void *pUserData)
{
    CLua *pSelf = ((CLua*)pUserData);
	if(!pSelf || !pSelf->Console())
		return;

	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Lua", "Lua files:");
	bool Files = false;
	for(int i = 0; i < MAX_LUA_FILES; i++)
	{
		if(pSelf->m_aLuaFiles[i][0] == 0)
			continue;
		char aBuf[256];
		str_copy(aBuf, pSelf->m_aLuaFiles[i], sizeof(aBuf));
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Lua", aBuf);
		Files = true;
	}
	if(!Files)
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Lua", "No Lua  files found");
}

void CLua::ConDeleteLuaFile(IConsole::IResult *pResult, void *pUserData)
{
    ((CLua*)pUserData)->DeleteLuaFile((char *)pResult->GetString(0));
}
void CLua::ConDeleteAllLuaFiles(IConsole::IResult *pResult, void *pUserData)
{
    ((CLua*)pUserData)->DeleteAllLuaFiles();
}

void CLua::DeleteLuaFile(int i)
{
    if (i >= 0 && i < MAX_LUA_FILES)
    {
        str_copy(m_aLuaFiles[i], "", sizeof(m_aLuaFiles[i]));
    }
}
void CLua::DeleteLuaFile(char *pFileDir)
{
	for (int i = 0; i < MAX_LUA_FILES; i++)
    {
        if (m_aLuaFiles[i] && str_comp_nocase(m_aLuaFiles[i], pFileDir)==0) //lua inactiv
        {
			str_copy(m_aLuaFiles[i], "", sizeof(m_aLuaFiles[i]));
        }
    }
}
void CLua::DeleteAllLuaFiles()
{
	for (int i = 0; i < MAX_LUA_FILES; i++)
		str_copy(m_aLuaFiles[i], "", sizeof(m_aLuaFiles[i]));
}

void CLua::ConfigSaveCallback(IConfig *pConfig, void *pUserData)
{
	CLua *pSelf = (CLua *)pUserData;
	char aBuf[128];
    for (int i = 0; i < MAX_LUA_FILES; i++)
    {
        const char *pEnd = aBuf+sizeof(aBuf)-4;
        str_copy(aBuf, "add_lua_file ", sizeof(aBuf));

        const char *pSrc = pSelf->m_aLuaFiles[i];
        char *pDst = aBuf+str_length(aBuf);
        *pDst++ = '"';
        while(*pSrc && pDst < pEnd)
        {
            if(*pSrc == '"' || *pSrc == '\\') // escape \ and "
                *pDst++ = '\\';
            *pDst++ = *pSrc++;
        }
        *pDst++ = '"';
        *pDst++ = 0;

        pConfig->WriteLine(aBuf);
    }
}
char *CLua::GetFileDir(int i)
{
    return m_aLuaFiles[i];
}

void CLua::Init()
{
	IConfig *pConfig = Kernel()->RequestInterface<IConfig>();
	if(pConfig)
		pConfig->RegisterCallback(ConfigSaveCallback, this);
	m_pConsole = Kernel()->RequestInterface<IConsole>();
	if(m_pConsole)
	{
		//m_pConsole->Register("add_lua_file", "s", CFGFLAG_SERVER, ConAddLuaFile, this, "Add a Lua file");
		m_pConsole->Register("delete_lua_file", "s", CFGFLAG_SERVER, ConDeleteLuaFile, this, "Delete a Lua file");
		m_pConsole->Register("delete_all_lua_files", "", CFGFLAG_SERVER, ConDeleteAllLuaFiles, this, "Delete all lua files");
		m_pConsole->Register("list_lua_files", "", CFGFLAG_SERVER, ConListLuaFiles, this, "List active Lua files");
	}
}

