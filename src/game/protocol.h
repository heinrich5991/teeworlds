/* (c) MAP94. See www.n-lvl.com/ndc/nclient/ for more information. */
#ifndef GAME_PROTOCOL_H
#define GAME_PROTOCOL_H
#include <engine/shared/network.h>

class CCPacket
{
public:
    CCPacket();
    ~CCPacket();

    unsigned short m_type;
    unsigned short m_size;
    unsigned int m_total_size;

    char *m_buffer;

    void SetBuffer(char *buf, unsigned int size);
    int GetSize();
    void ReadFromStream (char *buf);
    void WriteToStream (char *buf);

    void PrepareLevel0(char *pBuf, int BufSize, int Type0, int TotalSize0);
    void PrepareLevel1(char *pBuf, int BufSize, int Type0, int TotalSize0, int Type1, int TotalSize1);
    void PrepareLevel2(char *pBuf, int BufSize, int Type0, int TotalSize0, int Type1, int TotalSize1, int Type2, int TotalSize2);
    void PrepareLevel3(char *pBuf, int BufSize, int Type0, int TotalSize0, int Type1, int TotalSize1, int Type2, int TotalSize2, int Type3, int TotalSize3);
    bool SendData(CNetTCP *pSocket);


};
/*
Spezification:
if (Size < Total Size)
    Data => Swap

if (Total Size != Swap_size)
    Swap_error++;
    Delete Packet
    Send Error Packet with same type (including a checksum maybe)
    dbg_msg("Error", "Drop Packet @info")

if (Swap_error >= 3)
    Swap_error = 0
    Delete Swap
    Delete Packet // make sure that we don't fill in wrong data
    Send Error Packet with same type (including a checksum(packet string) maybe)
    dbg_msg("Error", "Delete Swap @info")
    dbg_msg("Error", "Drop Packet @info")

Sub-Packets COULD be stored in swap.
You CAN save up to 4 level packets in swap


*/

/*
General Info:

Header 8Byte
2       Byte    -   Type
2       Byte    -   Size
4       Byte    -   Total Size

Data
<= 4096 Byte    -   Data

IMPORTANT NOTE:
if you want to add an addon please use a Type > 60.000
and use sub Packets
You can register a type here: www.n-lvl.com/type.php
*/
#endif
