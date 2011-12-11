/* (c) MAP94. See www.n-lvl.com/ndc/nclient/ for more information. */
#ifndef GAME_CLIENT_MSGS_H
#define GAME_CLIENT_MSGS_H

#include "gameclient.h"
#include <game/client/ui.h>

class CNMsg
{
public:
    class CGameClient *m_pClient;
    CNMsg(class CGameClient *pClient);
    ~CNMsg();
    void Tick();

    class CPopupMsg
    {
    public:
        enum
        {
            POPUPSIZEDEFAULT = 0,
            POPUPSIZEMINIMAL = 1,
            POPUPSIZEMAXIMAL = 2,
        };
        CPopupMsg()
        {
            mem_zero(this, sizeof(CPopupMsg));
        }
        char m_aTitle[256];
        char m_aText[4096];
        bool m_Show;
        int m_Size;
        float m_Time;
        void (*m_CallBack)(void *pUser, CUIRect MainView);
        void *m_pCallBackUser;
    };
    CPopupMsg m_aMsgs[16];

    int m_Index;

    void AddMsg(CPopupMsg Msg);
};
#endif
