/* (c) MAP94. See www.n-lvl.com/ndc/nclient/ for more information. */

#include <base/system.h>
#include "protocol.h"

//std Packet
//m_type
//1     - system
//1.1   - login
//1.2   - register
//1.3   - status

//2     - chat
//2.1   - open new
//2.2.x - msg
//3 -
//4 -
//5 -
//6 -

CCPacket::CCPacket()
{
    m_buffer = 0;
    m_type = 0;
    m_size = 0;
    m_total_size = 0;
}

CCPacket::~CCPacket()
{
    if (m_buffer)
    {
        delete[] m_buffer;
    }
}

void CCPacket::ReadFromStream(const char *buf)
{
    mem_copy(&m_type, buf, sizeof(m_type));
    mem_copy(&m_size, buf + 2, sizeof(m_size));
    mem_copy(&m_total_size, buf + 4, sizeof(m_total_size));
    #if defined(CONF_ARCH_ENDIAN_BIG)
        swap_endian(&m_type, sizeof(m_type), 1);
        swap_endian(&m_size, sizeof(m_size), 1);
        swap_endian(&m_total_size, sizeof(m_total_size), 1);
    #endif
    if (m_size > 4096)
    {
        m_type = 0;
        m_size = 0;
        m_total_size = 0;
        return;
    }
    if (m_buffer)
    {
        delete[] m_buffer;
        m_buffer = 0;
    }
    m_buffer = new char[m_size + 1];
    mem_copy(m_buffer, buf + 8, m_size);
}

void CCPacket::SetBuffer(const char *buf, unsigned int size)
{
    if (size <= 0)
        size = str_length(buf) + 1;
    if (m_buffer)
    {
        delete[] m_buffer;
        m_buffer = 0;
        m_size = 0;
    }
    m_buffer = new char[size + 1];
    mem_copy(m_buffer, buf, size);
    m_size = size;
    if (size > m_total_size)
        m_total_size = size;
}

int CCPacket::GetSize()
{
    return sizeof(m_type) + sizeof(m_size) + sizeof(m_total_size) + m_size;
}

void CCPacket::WriteToStream(char *buf)
{
    mem_copy(buf, &m_type, sizeof(m_type));
    mem_copy(buf + 2, &m_size, sizeof(m_size));
    mem_copy(buf + 4, &m_total_size, sizeof(m_total_size));
    mem_copy(buf + 8, m_buffer, m_size);
    #if defined(CONF_ARCH_ENDIAN_BIG)
        swap_endian(&m_type, sizeof(m_type), 1);
        swap_endian(&m_size, sizeof(m_size), 1);
        swap_endian(&m_total_size, sizeof(m_total_size), 1);
    #endif
}

void CCPacket::PrepareLevel0(const char *pBuf, int BufSize, int Type0, int TotalSize0)
{
    SetBuffer(pBuf, BufSize);
    if (TotalSize0 > m_size)
        m_total_size = TotalSize0;
    if (Type0)
        m_type = Type0;
}

void CCPacket::PrepareLevel1(const char *pBuf, int BufSize, int Type0, int TotalSize0, int Type1, int TotalSize1)
{
    CCPacket TmpPacket;
    TmpPacket.PrepareLevel0(pBuf, BufSize, Type1, TotalSize1);
    char *TmpStream = new char[TmpPacket.GetSize()];
    TmpPacket.WriteToStream(TmpStream);


    SetBuffer(TmpStream, TmpPacket.GetSize());
    if (TotalSize0 > m_size)
        m_total_size = TotalSize0;
    if (Type0)
        m_type = Type0;

    delete []TmpStream;
}

void CCPacket::PrepareLevel2(const char *pBuf, int BufSize, int Type0, int TotalSize0, int Type1, int TotalSize1, int Type2, int TotalSize2)
{
    CCPacket TmpPacket;
    TmpPacket.PrepareLevel1(pBuf, BufSize, Type2, TotalSize2, Type1, TotalSize1);
    char *TmpStream = new char[TmpPacket.GetSize()];
    TmpPacket.WriteToStream(TmpStream);

    SetBuffer(TmpStream, TmpPacket.GetSize());
    if (TotalSize0 > m_size)
        m_total_size = TotalSize0;
    if (Type0)
        m_type = Type0;

    delete []TmpStream;
}

void CCPacket::PrepareLevel3(const char *pBuf, int BufSize, int Type0, int TotalSize0, int Type1, int TotalSize1, int Type2, int TotalSize2, int Type3, int TotalSize3)
{
    CCPacket TmpPacket;
    TmpPacket.PrepareLevel2(pBuf, BufSize, Type3, TotalSize3, Type2, TotalSize2, Type1, TotalSize1);
    char *TmpStream = new char[TmpPacket.GetSize()];
    TmpPacket.WriteToStream(TmpStream);

    SetBuffer(TmpStream, TmpPacket.GetSize());
    if (TotalSize0 > m_size)
        m_total_size = TotalSize0;
    if (Type0)
        m_type = Type0;

    delete []TmpStream;
}

bool CCPacket::SendData(CNetTCP *pSocket)
{
    if (pSocket->m_Status != 7)
    {
        return false;
    }
    char *TmpStream = new char[GetSize()];
    WriteToStream(TmpStream);
    int ret = pSocket->Send(TmpStream, GetSize());
    delete []TmpStream;
    return ret > 0;
}

