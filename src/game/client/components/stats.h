/* (c) MAP94. See www.n-lvl.com/ndc/nclient/ for more information. */
#ifndef GAME_CLIENT_COMPONENTS_STATS_H
#define GAME_CLIENT_COMPONENTS_STATS_H
#include <game/client/component.h>

class CStatsRecords
{
public:
    enum
    {
        RECORDROWDATASIZE = 32,
        RECORDROWSIZE = RECORDROWDATASIZE + 4,
    };
    struct CRecordRow
    {
        char m_aData[RECORDROWDATASIZE];
        int m_TimeStamp;
        CRecordRow *m_pNext;
    };
private:
    CRecordRow *m_pFirstRecords;
    CRecordRow *m_pLastRecords;
    int m_NumRecords;
public:
    int Add(char *pText);
    int GetSize();
    void Remove();
    void ToString(char *pDst, int DstSize);
};

class CStats : public CComponent
{
public:
    struct CStatsIndexRow
    {
        NETADDR m_ServerAddr;
        char m_aMap[256];
        char m_aGameType[32];
        char m_aServerName[256];
        int m_TimeStamp;
        int m_Uid;
    };
    enum
    {
        STATROW_INVALID = 0,
        STATROW_SERVER,
        STATROW_KILL,
        STATROW_FLAG,
        //STATROW_LUA,
    };
    enum
    {
        STATROW_SERVER_INVALID = 0,
        STATROW_SERVER_JOIN,
        STATROW_SERVER_LEAVE,
    };

    void ServerJoin();
    void ServerLeave();
    void OnMessage(int MsgType, void *pRawMsg);
    void OnStateChange(int NewState, int OldState);
	//void OnReset();
	void OnRender();
private:
    CStatsRecords m_Records;
    CStatsIndexRow m_IndexRow;

    bool m_Activ;
};

#endif

