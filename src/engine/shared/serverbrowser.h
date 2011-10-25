/* (c) heinrich5991. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.              */
#ifndef STATS_SERVERBROWSER_H
#define STATS_SERVERBROWSER_H

#include <base/system.h>
#include <engine/shared/network.h>
#include <engine/shared/protocol.h>

enum
{
	INFOVERSION_UNKNOWN=0,
	INFOVERSION_1,
	INFOVERSION_2
};

class CServerInfo
{
public:
	class CClient
	{
	public:
		char m_aName[MAX_NAME_LENGTH];
		char m_aClan[MAX_CLAN_LENGTH];
		int m_Country;
		int m_Score;
		int m_Player;
	};

	int m_MaxClients;
	int m_NumClients;
	int m_MaxPlayers;
	int m_NumPlayers;
	int m_Flags;
	int m_Progression;
	int m_Latency; // in ms
	char m_aGameType[16];
	char m_aName[64];
	char m_aMap[32];
	char m_aVersion[32];
	CClient m_aClients[MAX_CLIENTS];
	char m_aAddress[NETADDR_MAXSTRSIZE];

	int m_Version;
};


class CStatsServerBrowser
{
public:
	enum
	{
		MAX_ACTIONS=50,
		MAX_SERVERS_MISSING=5,
		MAX_TRIES=2,
		SERVERINFO_RETRY_TIME=10,
	};

	class CMasterServerEntry
	{
	public:
		NETADDR m_Addr;
		int64 m_RequestTime;
		int m_RequestTries;
		int64 m_LastLookup;
		int m_NumServers;
		int m_NumGotServers;
		int m_Done;
		char m_aHostname[64];

		CMasterServerEntry *m_pNext;
	};

	class CServerEntry
	{
	public:
		NETADDR m_Addr;
		int m_InfoVersion;
		int64 m_RequestTime;
		int m_RequestToken;
		int m_RequestTries;
		int m_GotInfo;
		int m_Done;
		CServerInfo m_Info;

		CServerEntry *m_pNext;
	};

	CStatsServerBrowser();
	~CStatsServerBrowser();

	void Refresh();
	int Update(int MaxActions = 0);
	int IsRefreshing() { return m_Refreshing; }

	int NumServers() const { return m_NumServers; }
	int NumReceivedServers() const { return m_NumReceivedServers; }
	int NumNotReceivedServers() const { return m_NumServers - m_NumReceivedServers; };

	void AddMaster(const char *pHostname);
	
	const NETADDR *ReceivedServer(int Index) const;
	const NETADDR *NotReceivedServer(int Index) const;
	const NETADDR *Server(int Index) const;

	const CServerInfo *ServerInfo(int Index) const;

private:
	void ClearServers();
	CServerEntry *Find(const NETADDR *pAddr);
	void ProcessPacket(const CNetChunk *pPacket);
	int UpdateMasters();
	int RequestNextServerInfo();
	CMasterServerEntry *FindMaster(const NETADDR *pAddr);
	void AddServer(const NETADDR *pAddr, int InfoVersion);
	int Done() const;
	void ResetMasterServers();
	void CheckMasterDone(CMasterServerEntry *pMasterServer);
	int ReadInfo(const char *pData, int DataSize, CServerInfo *pInfo, int InfoVersion, const char *pAddressStr, int RequestToken);

	CServerEntry *m_pServers;
	CServerEntry *m_pServersLast;
	int m_NumServers;
	int m_NumReceivedServers;
	
	int m_MasterServersDone;
	CMasterServerEntry *m_pMasterServers;
	CMasterServerEntry *m_pMasterServersLast;
	
	int m_Refreshing;
	CNetClient m_Net;
};

#endif

