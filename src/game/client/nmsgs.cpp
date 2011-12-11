/* (c) MAP94. See www.n-lvl.com/ndc/nclient/ for more information. */
#include "nmsgs.h"

CNMsg::CNMsg(class CGameClient *pClient)
{
    m_pClient = pClient;
    mem_zero(m_aMsgs, sizeof(m_aMsgs));
    m_Index = 0;
}

CNMsg::~CNMsg()
{

}

void CNMsg::Tick()
{

}

void CNMsg::AddMsg(CPopupMsg Msg)
{
    if (m_Index >= 16)
        return;
    m_aMsgs[m_Index] = Msg;
    m_Index++;
}
