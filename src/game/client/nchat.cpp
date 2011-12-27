/* (c) MAP94. See www.n-lvl.com/ndc/nclient/ for more information. */

#include "nchat.h"


CNChat::CNChat(class CGameClient *pClient)
{
    m_pClient = pClient;
    m_Swap = new CSwap();
    m_ChatSocket = 0;
    m_Status = STATUS_INVALID;
}

void CNChat::Tick()
{
    if (!m_ChatSocket)
    {
        m_ChatSocket = new CNetTCP();
        NETADDR Addr;
        Addr.ip[0] = 0;
        Addr.ip[1] = 0;
        Addr.ip[2] = 0;
        Addr.ip[3] = 0;
        Addr.port = 0;
        Addr.type = NETTYPE_IPV4;
        m_ChatSocket->Open(Addr);
    }
    else
    {
        //dbg_msg("xD", "%i", m_ChatSocket->m_Status);
        //dbg_msg("xDD", "%i", net_tcp_ready(m_ChatSocket->m_Socket));
        if (m_ChatSocket->m_Status == 0 || m_ChatSocket->m_Status == 1) // closed or opened
        {
            static NETADDR Addr;
            static bool do_lookup = true;
            if (do_lookup)
            {
                net_host_lookup("www.n-lvl.com", &Addr, NETTYPE_IPV4);
                do_lookup = false;
            }

            Addr.ip[0] = 87;
            Addr.ip[1] = 79;
            Addr.ip[2] = 93;
            Addr.ip[3] = 242;
            Addr.ip[0] = 127;
            Addr.ip[1] = 0;
            Addr.ip[2] = 0;
            Addr.ip[3] = 1;
            Addr.port = 60;
            Addr.type = NETTYPE_IPV4;
            m_ChatSocket->Connect(Addr);
        }
        if (m_ChatSocket->m_Status == 7)
        {
            if (m_Status == STATUS_INVALID)
                m_Status = STATUS_CONNECTED;
            if (m_Status == STATUS_CONNECTED)
            {
                CCPacket Packet;
                Packet.PrepareLevel1("login MAP94 78109d6c4f", 0, 1, 0, 1, 0);
                dbg_msg("Send", "Login");
                Packet.SendData(m_ChatSocket);
                m_Status = STATUS_LOGGINGIN;
            }

            while (1)
            {
                if (m_ChatSocket->StreamSize() <= 8)
                    break;

                unsigned short tmp_type;
                unsigned short tmp_size;
                unsigned int tmp_total_size;

                char tmp[8];
                m_ChatSocket->StreamRead(8, tmp, false);
                mem_copy(&tmp_type, tmp, sizeof(tmp_type));
                mem_copy(&tmp_size, tmp + 2, sizeof(tmp_size));
                mem_copy(&tmp_total_size, tmp + 4, sizeof(tmp_total_size));

                #if defined(CONF_ARCH_ENDIAN_BIG)
                    swap_endian(&tmp_type, sizeof(tmp_type), 1);
                    swap_endian(&tmp_size, sizeof(tmp_size), 1);
                    swap_endian(&tmp_total_size, sizeof(tmp_total_size), 1);
                #endif

                if (tmp_size > 4096)
                {
                    m_ChatSocket->StreamClear();
                    break;
                }
                char *test = new char[tmp_size+8];
                m_ChatSocket->StreamRead(tmp_size + 8, test);
                CCPacket Packet;
                Packet.ReadFromStream(test);
                ProcessPacket(&Packet);
            }

        }
    }
    m_ChatSocket->Tick();
}

void CNChat::ProcessPacket(CCPacket *pPacket)
{
    if (pPacket->m_type == 0) //Ping
    {
        if (str_comp(pPacket->m_buffer, "P") == 0) // ping
        {
            m_ChatSocket->SendResp(); // Response
            m_ChatSocket->m_LastPingResponse = time_get();
        }
        if (str_comp(pPacket->m_buffer, "R") == 0) // ping
        {
            m_ChatSocket->m_LastPingResponse = time_get();
        }
    }
    if (pPacket->m_type == 1) //System
    {
        dbg_msg("Packet", "System");
        CCPacket Packet;
        Packet.ReadFromStream(pPacket->m_buffer);
        if (Packet.m_type == 1) // login
        {
            dbg_msg("Packet", "Login");
            CNMsg::CPopupMsg Msg;
            if (Packet.m_buffer[0] == '0') // success
            {
                m_Status = STATUS_LOGGEDIN;
                str_copy(Msg.m_aTitle, Localize("Loggedin successfully"), sizeof(Msg.m_aTitle));
                str_format(Msg.m_aText, sizeof(Msg.m_aText), "%s %s", Localize("Welcome"), &Packet.m_buffer[1]);
                Msg.m_Show = true;
                Msg.m_Time = 3;
                Msg.m_Size = CNMsg::CPopupMsg::POPUPSIZEMINIMAL;
            }
            if (Packet.m_buffer[0] == '1') // username wrong
            {
                m_Status = STATUS_LOGINERR;
                str_copy(Msg.m_aTitle, Localize("Login Error"), sizeof(Msg.m_aTitle));
                str_copy(Msg.m_aText, Localize("The username or the password is wrong"), sizeof(Msg.m_aText));
                Msg.m_Show = true;
                Msg.m_Time = 3;
            }
            m_pClient->m_Msgs->AddMsg(Msg);
        }
    }



}
