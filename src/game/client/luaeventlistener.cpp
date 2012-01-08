#include "lua.h"

CLuaEventListener::CLuaEventListener()
{
    m_aListeners.clear();
}

CLuaEventListener::~CLuaEventListener()
{
    m_aListeners.clear();
}


void CLuaEventListener::AddEventListener(class CLuaFile *pLuaFile, char *pEvent, char *pLuaFunction)
{
    CLuaListenerData Listener;
    Listener.m_pLuaFile = pLuaFile;
    str_copy(Listener.m_aLuaFunction, pLuaFunction, sizeof(Listener.m_aLuaFunction));
    str_copy(Listener.m_aEvent, pEvent, sizeof(Listener.m_aEvent));
    m_aListeners.add(Listener);
}

void CLuaEventListener::OnEvent(const char *pEvent)
{
	for(array<CLuaListenerData>::range r = m_aListeners.all(); !r.empty(); r.pop_front())
    {
        if (r.front().m_aEvent && str_comp(r.front().m_aEvent, pEvent) == 0)
        {
            if (r.front().m_pLuaFile->FunctionExist(r.front().m_aLuaFunction))
            {
                r.front().m_pLuaFile->FunctionExec(r.front().m_aLuaFunction);
            }
        }
    }
}

void CLuaEventListener::RemoveEventListener(class CLuaFile *pLuaFile, char *pEvent)
{
	for(array<CLuaListenerData>::range r = m_aListeners.all(); !r.empty(); r.pop_front())
    {
        if (r.front().m_pLuaFile == pLuaFile && str_comp(r.front().m_aEvent, pEvent) == 0)
        {
            m_aListeners.remove(r.front());
        }
    }
}

void CLuaEventListener::RemoveAllEventListeners(class CLuaFile *pLuaFile)
{
	for(array<CLuaListenerData>::range r = m_aListeners.all(); !r.empty(); r.pop_front())
    {
        if (r.front().m_pLuaFile == pLuaFile)
        {
            m_aListeners.remove(r.front());
        }
    }
}
