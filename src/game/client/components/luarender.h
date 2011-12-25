#ifndef GAME_CLIENT_COMPONENTS_LUARENDER_H
#define GAME_CLIENT_COMPONENTS_LUARENDER_H
#include <game/client/component.h>

class CLuaRender : public CComponent
{
	int m_Level;
	char m_aEventString[32];
public:
	CLuaRender(int Level);
	virtual void OnRender();
};

#endif

