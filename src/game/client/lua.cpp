/* (c) MAP94. See www.n-lvl.com/ndc/nclient/ for more information. */
#include "lua.h"
#include "string.h"
#include "components/flow.h"
#include "components/particles.h"
#include <game/generated/client_data.h>
#include <engine/serverbrowser.h>
#include <engine/textrender.h>
#include <engine/sound.h>
#include <game/client/lineinput.h>
#include <game/client/components/menus.h>
#include <game/client/components/chat.h>

int StrIsInteger(const char *pStr)
{
	while(*pStr)
	{
		if(!(*pStr >= '0' && *pStr <= '9'))
			return 0;
		pStr++;
	}
	return 1;
}

int StrIsFloat(const char *pStr)
{
	bool Dot = false;
	while(*pStr)
	{
		if(*pStr < '0' || *pStr > '9')
		{
			if(!Dot && *pStr == '.')
				Dot = true;
			else
				return 0;
		}
		pStr++;
	}
	return 1;
}

void CLua::Tick()
{
    for (int i = 0; i < MAX_LUA_FILES; i++)
    {
        if (m_aLuaFiles[i].GetScriptName()[0] == 0 && m_pClient->m_pLuaCore->GetFileDir(i)[0])
            m_aLuaFiles[i].Init(m_pClient->m_pLuaCore->GetFileDir(i));
        else if (m_aLuaFiles[i].GetScriptName()[0] && m_pClient->m_pLuaCore->GetFileDir(i)[0] == 0)
            m_aLuaFiles[i].Close();
        else if (m_aLuaFiles[i].GetScriptName()[0])
            m_aLuaFiles[i].Tick();
    }
}

void CLua::End()
{
    for (int i = 0; i < MAX_LUA_FILES; i++)
    {
        m_aLuaFiles[i].End();
    }
}

CLua::CLua(CGameClient *pClient)
{
    mem_zero(this, sizeof(CLua));
    Close();

    m_pClient = pClient;

    for (int i = 0; i < MAX_LUA_FILES; i++)
    {
        m_aLuaFiles[i].m_pClient = pClient;
        m_aLuaFiles[i].m_pLuaHandler = this;
    }

    m_OriginalGameTexture = g_pData->m_aImages[IMAGE_GAME].m_Id;
}

CLua::~CLua()
{
    Close();
}

void CLua::Close()
{
    End();
}

int CLua::GetFileId(char *pFileDir)
{
    for (int i = 0; i < MAX_LUA_FILES; i++)
    {
        if (str_comp(m_aLuaFiles[i].GetScriptName(), pFileDir) == 0)
            return i;
    }
    return -1;
}

void CLua::ConfigClose(char *pFileDir)
{
    int Id = GetFileId(pFileDir);
    if (Id == -1)
        return;
    m_aLuaFiles[Id].ConfigClose();
}
