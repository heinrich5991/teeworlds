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

void CLua::AddLuaFile(char *pFilename)
{
    for (int i = 0; i < MAX_LUA_FILES; i++)
    {
        if (m_aLuaFiles[i][0] == 0 && str_comp(m_aLuaFiles[i], pFilename)) //lua inactiv
        {
            str_copy(m_aLuaFiles[i], pFilename, sizeof(m_aLuaFiles[i]));
            break;
        }
    }
}

void CLua::DeleteLuaFile(int i)
{
    if (i >= 0 && i < MAX_LUA_FILES)
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
char *CLua::GetFileName(int i)
{
    return m_aLuaFiles[i];
}

void CLua::Init()
{
	IConfig *pConfig = Kernel()->RequestInterface<IConfig>();
	if(pConfig)
		pConfig->RegisterCallback(ConfigSaveCallback, this);
	IConsole *pConsole = Kernel()->RequestInterface<IConsole>();
	if(pConsole)
	{
		pConsole->Register("add_lua_file", "s", CFGFLAG_SERVER, ConAddLuaFile, this, "Add a Lua file");
	}
}

