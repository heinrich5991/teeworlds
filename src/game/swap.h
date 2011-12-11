/* (c) MAP94. See www.n-lvl.com/ndc/nclient/ for more information. */
#ifndef GAME_SWAP_H
#define GAME_SWAP_H
#include <base/system.h>

class CSwap
{
public:
    typedef struct
    {
        unsigned long long m_Uid;
        char *m_pBuffer;
        unsigned int m_UsedSize;
        unsigned int m_MaxSize;
        unsigned int m_Errors;
    } SWAP;

    CSwap();
    unsigned long long Uid_Create(unsigned short t1, unsigned short t2 = 0, unsigned short t3 = 0, unsigned short t4 = 0);
    void Uid_Get_Type(unsigned long long Uid, unsigned short *pt1 = 0, unsigned short *pt2 = 0, unsigned short *pt3 = 0, unsigned short *pt4 = 0);

    bool Exist(unsigned long long Uid);
    bool Read(unsigned long long Uid, char *Buffer, int BufferSize);                //returns false if the uid does not exist
    bool Write(unsigned long long Uid, char *Buffer, int BufferSize, unsigned int MaxSize);  //returns false if maxsize != m_swap->maxsize
    bool Create(unsigned long long Uid, int MaxSize);
    int GetUsedSize(unsigned long long Uid); // returns -1 if the give uid does not exist
    int GetMaxSize(unsigned long long Uid); // returns -1 if the give uid does not exist
    int GetErrors(unsigned long long Uid); // returns -1 if the give uid does not exist
    bool Destroy(unsigned long long Uid);
    SWAP m_Swap[4096];
private:
    int GetIndexByUid(unsigned long long Uid);
    int GetEmptyIndex();
    bool FreeIndexByUid(unsigned long long Uid);
};


#endif
