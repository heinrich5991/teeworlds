/* (c) MAP94. See www.n-lvl.com/ndc/nclient/ for more information. */
#ifndef GAME_CLIENT_NCHAT_H
#define GAME_CLIENT_NCHAT_H
#include "gameclient.h"
#include <game/protocol.h>
#include <game/swap.h>
#include <engine/shared/network.h>

class CNChat
{
public:
    CNChat(class CGameClient *pClient);
    void Tick();
    void ProcessPacket(CCPacket *pPacket);
    CSwap *m_Swap;
    class CGameClient *m_pClient;







    enum CStatus
    {
        STATUS_INVALID = 0,
        STATUS_CONNECTED,
        STATUS_LOGGINGIN,
        STATUS_LOGGEDIN,
        STATUS_LOGINERR,
        STATUS_LOADING_CONTACT_LIST,
    } m_Status;









	//Chat TCP
	CNetTCP *m_ChatSocket;
};

#endif
