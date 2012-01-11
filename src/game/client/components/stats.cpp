/* (c) MAP94. See www.n-lvl.com/ndc/nclient/ for more information. */
#include "stats.h"
#include <engine/serverbrowser.h>

int CStatsRecords::Add(char *pText)
{
    CRecordRow *tmp = new CRecordRow;
    tmp->m_pNext = 0;
    if (m_pLastRecords)
    {
        m_pLastRecords->m_pNext = tmp;
    }
    else
    {
        m_pFirstRecords = tmp;
    }
    m_pLastRecords = tmp;
    str_copy(tmp->m_aData, pText, sizeof(tmp->m_aData));
    tmp->m_TimeStamp = time_timestamp();
    m_NumRecords++;
    return m_NumRecords;
}

void CStatsRecords::ToString(char *pDst, int DstSize)
{
    int Size = 0;
    CRecordRow *Tmp = m_pFirstRecords;
    for (Size = 0; Size < DstSize && Tmp; Tmp = Tmp->m_pNext, Size = Size + RECORDROWSIZE)
    {
        mem_copy(pDst + Size, Tmp, RECORDROWSIZE);
    }
}

int CStatsRecords::GetSize()
{
    return m_NumRecords * RECORDROWSIZE;
}

void CStatsRecords::Remove()
{
    if (!m_pFirstRecords)
        return;
    CRecordRow *Tmp = m_pFirstRecords;
    CRecordRow *TmpRmv = 0;
    while (Tmp = Tmp->m_pNext)
    {
        if (TmpRmv)
        {
            delete TmpRmv;
        }
        TmpRmv = Tmp;
    }
    delete Tmp;
    delete m_pFirstRecords;
    m_pFirstRecords = 0;
    m_pLastRecords = 0;
    m_NumRecords = 0;
}




void CStats::OnMessage(int MsgType, void *pRawMsg)
{
	if(MsgType == NETMSGTYPE_SV_KILLMSG)
	{
		CNetMsg_Sv_KillMsg *pMsg = (CNetMsg_Sv_KillMsg *)pRawMsg;
        char aMsg[CStatsRecords::RECORDROWDATASIZE] = {0};
        aMsg[0] = STATROW_KILL;
        mem_copy(&aMsg[1], pMsg, sizeof(CNetMsg_Sv_KillMsg));
        m_Records.Add(aMsg);
	}
    else if(MsgType == NETMSGTYPE_SV_CHAT)
	{
		CNetMsg_Sv_Chat *pMsg = (CNetMsg_Sv_Chat *)pRawMsg;
		if(pMsg->m_ClientID < 0)
		{
			const char *pHeystack;
			const char *pNeedle = "flag was captured by ";
			if(pHeystack = str_find_nocase(pMsg->m_pMessage, pNeedle))
			{
				char aName[64];
				pHeystack += str_length(pNeedle);
				str_copy(aName, pHeystack, sizeof(aName));
				if(str_comp_nocase(aName + str_length(aName) - 9, " seconds)") == 0)
				{
					char *c = aName + str_length(aName)-10;
					while(c > aName)
					{
						c--;
						if(*c == '(')
						{
							*(c-1) = 0;
							break;
						}
					}
				}
				if (str_comp_nocase(m_pClient->m_aClients[m_pClient->m_Snap.m_LocalClientID].m_aName, aName) == 0)
                {
                    char aMsg[CStatsRecords::RECORDROWDATASIZE] = {0};
                    aMsg[0] = STATROW_FLAG;
                    m_Records.Add(aMsg);
                }
			}
		}
	}
}

void CStats::OnRender()
{
    CServerInfo CurrentServerInfo;
	Client()->GetServerInfo(&CurrentServerInfo);
    if (m_Activ == false && CurrentServerInfo.m_aName[0])
    {
        ServerJoin();
        m_Activ = true;
    }
}

void CStats::OnStateChange(int NewState, int OldState)
{
    if (NewState == IClient::STATE_ONLINE)
        m_Activ = false;
    if (OldState == IClient::STATE_ONLINE && NewState < IClient::STATE_ONLINE)
    {
        ServerLeave();
        m_Activ = false;
    }
}

void CStats::ServerJoin()
{
    CServerInfo CurrentServerInfo;
	Client()->GetServerInfo(&CurrentServerInfo);

    //set default
    m_IndexRow.m_Uid = 0;

    //load last
    char aFilePath[1024];
    fs_storage_path("Teeworlds", aFilePath, sizeof(aFilePath));
    str_append(aFilePath, "/stats/index.stat", sizeof(aFilePath));
    IOHANDLE IndexFile = io_open(aFilePath, IOFLAG_READ);
    if (IndexFile)
    {
        while(io_read(IndexFile, &m_IndexRow, sizeof(m_IndexRow)))
        {
            //do nothing
        }
        io_close(IndexFile);
    }


    //auto increment
    m_IndexRow.m_Uid++;
    m_IndexRow.m_ServerAddr = CurrentServerInfo.m_NetAddr;
    str_copy(m_IndexRow.m_aMap, CurrentServerInfo.m_aMap, sizeof(m_IndexRow.m_aMap));
    str_copy(m_IndexRow.m_aGameType, CurrentServerInfo.m_aGameType, sizeof(m_IndexRow.m_aGameType));
    str_copy(m_IndexRow.m_aServerName, CurrentServerInfo.m_aName, sizeof(m_IndexRow.m_aServerName));
    m_IndexRow.m_TimeStamp = time_timestamp();
    m_Records.Remove();

    char aMsg[CStatsRecords::RECORDROWDATASIZE] = {0};
    aMsg[0] = STATROW_SERVER;
    aMsg[1] = STATROW_SERVER_JOIN;
    aMsg[2] = m_pClient->m_Snap.m_LocalClientID;
    m_Records.Add(aMsg);
}

void CStats::ServerLeave()
{
    if (!m_Activ)
        return;
    char aMsg[CStatsRecords::RECORDROWDATASIZE] = {0};
    aMsg[0] = STATROW_SERVER;
    aMsg[1] = STATROW_SERVER_LEAVE;
    m_Records.Add(aMsg);

    char *pData = new char[m_Records.GetSize()];
    m_Records.ToString(pData, m_Records.GetSize());

    char aFilePath[1024];
    fs_storage_path("Teeworlds", aFilePath, sizeof(aFilePath));
    str_append(aFilePath, "/stats/index.stat", sizeof(aFilePath));
    IOHANDLE IndexFile = io_open(aFilePath, IOFLAG_APPEND);
    if(IndexFile)
    {
        io_write(IndexFile, &m_IndexRow, sizeof(m_IndexRow));
        io_close(IndexFile);
    }
    else
        dbg_msg("stats", "failed to open index file (%s)", aFilePath);

    //write data
    fs_storage_path("Teeworlds", aFilePath, sizeof(aFilePath));
    str_format(aFilePath, sizeof(aFilePath), "%s/stats/%i.stat", aFilePath, m_IndexRow.m_Uid);
    IOHANDLE test = io_open(aFilePath, IOFLAG_WRITE);
    if(test)
    {
        io_write(test, pData, m_Records.GetSize());
        io_close(test);
    }
    else
        dbg_msg("stats", "failed to open stats file (%s)", aFilePath);
    //save index
    //save data
}
