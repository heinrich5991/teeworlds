/* (c) MAP94. See www.n-lvl.com/ndc/nclient/ for more information. */
#include "swap.h"

int min(int a, int b)
{
    if (a > b)
        return b;
    else
        return a;
}

CSwap::CSwap()
{
    mem_zero(m_Swap, sizeof(m_Swap));
}

unsigned long long CSwap::Uid_Create(unsigned short t1, unsigned short t2, unsigned short t3, unsigned short t4)
{
    unsigned long long uid = 0;
    ((unsigned short *)(&uid))[0] = t1;
    ((unsigned short *)(&uid))[1] = t2;
    ((unsigned short *)(&uid))[2] = t3;
    ((unsigned short *)(&uid))[3] = t4;
    return uid;
}

void CSwap::Uid_Get_Type(unsigned long long Uid, unsigned short *pt1, unsigned short *pt2, unsigned short *pt3, unsigned short *pt4)
{
    if (pt1)
        pt1[0] = ((unsigned short *)(&Uid))[0];
    if (pt2)
        pt2[0] = ((unsigned short *)(&Uid))[1];
    if (pt3)
        pt3[0] = ((unsigned short *)(&Uid))[2];
    if (pt4)
        pt4[0] = ((unsigned short *)(&Uid))[3];
}

bool CSwap::Exist(unsigned long long Uid)
{
    if (GetIndexByUid(Uid) != -1)
        return true;
    else
        return false;
}

int CSwap::GetUsedSize(unsigned long long Uid)
{
    int i = GetIndexByUid(Uid);
    if (i != -1)
        return m_Swap[i].m_UsedSize;
    else
        return -1;
}

int CSwap::GetMaxSize(unsigned long long Uid)
{
    int i = GetIndexByUid(Uid);
    if (i != -1)
        return m_Swap[i].m_MaxSize;
    else
        return -1;
}

int CSwap::GetErrors(unsigned long long Uid)
{
    int i = GetIndexByUid(Uid);
    if (i != -1)
        return m_Swap[i].m_Errors;
    else
        return -1;
}

bool CSwap::Create(unsigned long long Uid, int MaxSize)
{
    if (Exist(Uid))
        return false;
    int i = GetEmptyIndex();
    if (i == -1)
        return false;
    m_Swap[i].m_Uid = Uid;
    m_Swap[i].m_Errors = 0;
    m_Swap[i].m_MaxSize = MaxSize;
    m_Swap[i].m_pBuffer = new char[MaxSize];
    mem_zero(m_Swap[i].m_pBuffer, MaxSize);
    m_Swap[i].m_UsedSize = 0;
    return true;
}

bool CSwap::Read(unsigned long long Uid, char *Buffer, int BufferSize)
{
    int i = GetIndexByUid(Uid);
    if (i == -1)
        return false;
    mem_copy(Buffer, m_Swap[i].m_pBuffer, min(BufferSize, m_Swap[i].m_UsedSize));
    return true;
}

bool CSwap::Write(unsigned long long Uid, char *Buffer, int BufferSize, unsigned int MaxSize)
{
    int i = GetIndexByUid(Uid);
    if (i == -1)
        return false;
    if (MaxSize != m_Swap[i].m_MaxSize)
    {
        m_Swap[i].m_Errors++;
        return false;
    }
    mem_copy(m_Swap[i].m_pBuffer + m_Swap[i].m_UsedSize, Buffer, min(MaxSize - m_Swap[i].m_UsedSize, BufferSize));
    m_Swap[i].m_UsedSize += min(MaxSize - m_Swap[i].m_UsedSize, BufferSize);
    return true;
}

int CSwap::GetIndexByUid(unsigned long long Uid)
{
    if (Uid == 0ull)
        return -1;
    for (int i = 0; i < 4096; i++)
    {
        if (m_Swap[i].m_Uid == Uid)
        {
            return i;
        }
    }
    return -1;
}

int CSwap::GetEmptyIndex()
{
    for (int i = 0; i < 4096; i++)
    {
        if (m_Swap[i].m_Uid == 0)
        {
            return i;
        }
    }
    return -1;
}

bool CSwap::Destroy(unsigned long long Uid)
{
    return FreeIndexByUid(Uid);
}

bool CSwap::FreeIndexByUid(unsigned long long Uid)
{
    int i = GetIndexByUid(Uid);
    if (i == -1)
        return false;
    m_Swap[i].m_Errors = 0;
    m_Swap[i].m_MaxSize = 0;
    delete[] m_Swap[i].m_pBuffer;
    m_Swap[i].m_pBuffer = 0;
    m_Swap[i].m_Uid = 0;
    m_Swap[i].m_UsedSize = 0;
    return true;
}
