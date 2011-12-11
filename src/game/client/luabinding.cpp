/* (c) MAP94. See www.n-lvl.com/ndc/nclient/ for more information. */
#include "lua.h"
#include "components/flow.h"
#include "components/particles.h"
#include <game/generated/client_data.h>
#include <engine/serverbrowser.h>
#include <engine/textrender.h>
#include <engine/sound.h>
#include <game/client/lineinput.h>
#include <game/client/components/menus.h>
#include <game/client/components/chat.h>

CLuaBinding::CLuaBinding(CGameClient *pClient)
{
    mem_zero(this, sizeof(CLuaBinding));
    m_pClient = pClient;
}
