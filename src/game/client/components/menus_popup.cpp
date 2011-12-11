#include "menus.h"

void CMenus::RenderPopus(CUIRect MainView)
{
    static int64 sLastTime = time_get();
    if (m_pClient->m_Msgs->m_aMsgs[0].m_Show == false)
    {
        sLastTime = time_get();
        return;
    }
    MainView.VSplitRight(300.0f, 0, &MainView);
    if (m_pClient->m_Msgs->m_aMsgs[0].m_Size == CNMsg::CPopupMsg::POPUPSIZEMINIMAL)
        MainView.HSplitBottom(50.0f, 0, &MainView);
    if (m_pClient->m_Msgs->m_aMsgs[0].m_Size == CNMsg::CPopupMsg::POPUPSIZEDEFAULT)
        MainView.HSplitBottom(75.0f, 0, &MainView);
    if (m_pClient->m_Msgs->m_aMsgs[0].m_Size == CNMsg::CPopupMsg::POPUPSIZEDEFAULT)
        MainView.HSplitBottom(100.0f, 0, &MainView);
	RenderTools()->DrawUIRect(&MainView, vec4(0,0,0,0.5f), CUI::CORNER_ALL, 10.0f);

    m_pClient->m_Msgs->m_aMsgs[0].m_Time -= (float)(time_get() - sLastTime) / time_freq();
    sLastTime = time_get();

    if (m_pClient->m_Msgs->m_aMsgs[0].m_CallBack)
    {
        m_pClient->m_Msgs->m_aMsgs[0].m_CallBack(m_pClient->m_Msgs->m_aMsgs[0].m_pCallBackUser, MainView);
    }
    else
    {
        MainView.Margin(5.0f, &MainView);
        CUIRect Title;
        CUIRect Text;
        MainView.HSplitTop(15.0f, &Title, &Text);
        Text.HSplitTop(5.0f, 0, &Text);
        RenderTools()->UI()->DoLabel(&Title, m_pClient->m_Msgs->m_aMsgs[0].m_aTitle, 15.0f, 0);
        RenderTools()->UI()->DoLabel(&Text, m_pClient->m_Msgs->m_aMsgs[0].m_aText, 15.0f, -1, Text.w);
    }


    if (m_pClient->m_Msgs->m_aMsgs[0].m_Time <= 0)
    {
        m_pClient->m_Msgs->m_aMsgs[0].m_Show = false;
        m_pClient->m_Msgs->m_Index--;
        for (int i = 0; i < 15; i++)
        {
            m_pClient->m_Msgs->m_aMsgs[i] = m_pClient->m_Msgs->m_aMsgs[i + 1];
        }
    }
    //ui
}
