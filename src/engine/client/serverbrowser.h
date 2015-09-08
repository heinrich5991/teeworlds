/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_CLIENT_SERVERBROWSER_H
#define ENGINE_CLIENT_SERVERBROWSER_H

#include <engine/external/http_parser/http_parser.h>

#include <engine/serverbrowser.h>

class CServerBrowser : public IServerBrowser
{
public:
	class CServerEntry
	{
	public:
		enum
		{
			STATE_INVALID=0,
			STATE_PENDING,
			STATE_READY,
			MAX_TOKEN=0xFF
		};

		NETADDR m_Addr;
		int64 m_RequestTime;
		int m_InfoState;
		int m_CurrentToken;	// the token is to keep server refresh separated from each other
		CServerInfo m_Info;

		CServerEntry *m_pNextIp; // ip hashed list

		CServerEntry *m_pPrevReq; // request list
		CServerEntry *m_pNextReq;
	};

	enum
	{
		MAX_FAVORITES=256
	};

	CServerBrowser();

	// interface functions
	void Refresh(int Type);
	bool IsRefreshing() const;
	bool IsRefreshingMasters() const;
	int LoadingProgression() const;

	int NumServers() const { return m_NumServers; }

	int NumSortedServers() const { return m_NumSortedServers; }
	const CServerInfo *SortedGet(int Index) const;

	bool IsFavorite(const NETADDR &Addr) const;
	void AddFavorite(const NETADDR &Addr);
	void RemoveFavorite(const NETADDR &Addr);

	//
	void Update(bool ForceResort);
	void Set(const NETADDR &Addr, int Type, int Token, const CServerInfo *pInfo);
	void Request(const NETADDR &Addr) const;

	void SetBaseInfo(class CNetClient *pClient, const char *pNetVersion);

private:
	enum
	{
		MASTERSTATE_NONE=0,
		MASTERSTATE_CONNECTING,
		MASTERSTATE_AWAITING_RESPONSE,
	};

	struct CReceiveCallbackData
	{
		char m_aBuf[1024];
		size_t m_BufSize;
		bool m_Invalid;
		CServerBrowser *m_pThis;
	};

	struct CMaster
	{
		NETSOCKET m_Socket;
		int m_State;
		char m_aRequest[1024];
		int m_RequestSize;
		int m_RequestSentSize;
		http_parser m_Parser;
		CReceiveCallbackData m_CallbackData;
	};

	CNetClient *m_pNetClient;
	CMaster m_aMasters[IMasterServer::MAX_MASTERSERVERS];
	IMasterServer *m_pMasterServer;
	class IConsole *m_pConsole;
	class IFriends *m_pFriends;
	char m_aNetVersion[128];

	CHeap m_ServerlistHeap;
	CServerEntry **m_ppServerlist;
	int *m_pSortedServerlist;

	NETADDR m_aFavoriteServers[MAX_FAVORITES];
	int m_NumFavoriteServers;

	CServerEntry *m_aServerlistIp[256]; // ip hash list

	CServerEntry *m_pFirstReqServer; // request list
	CServerEntry *m_pLastReqServer;
	int m_NumRequests;

	int m_NeedRefresh;

	int m_NumSortedServers;
	int m_NumSortedServersCapacity;
	int m_NumServers;
	int m_NumServerCapacity;

	int m_Sorthash;
	char m_aFilterString[64];
	char m_aFilterGametypeString[128];

	// the token is to keep server refresh separated from each other
	int m_CurrentLanToken;

	int m_ServerlistType;
	int64 m_BroadcastTime;

	http_parser_settings m_HttpParserSettings;

	// sorting criterions
	bool SortCompareName(int Index1, int Index2) const;
	bool SortCompareMap(int Index1, int Index2) const;
	bool SortComparePing(int Index1, int Index2) const;
	bool SortCompareGametype(int Index1, int Index2) const;
	bool SortCompareNumPlayers(int Index1, int Index2) const;
	bool SortCompareNumClients(int Index1, int Index2) const;

	//
	void Filter();
	void Sort();
	int SortHash() const;

	CServerEntry *Find(const NETADDR &Addr);
	CServerEntry *Add(const NETADDR &Addr);

	void RemoveRequest(CServerEntry *pEntry);
	void QueueRequest(CServerEntry *pEntry);

	void RequestImpl(const NETADDR &Addr, CServerEntry *pEntry) const;

	void SetInfo(CServerEntry *pEntry, const CServerInfo &Info);

	static void ConfigSaveCallback(IConfig *pConfig, void *pUserData);

	static int ServerListCallback(http_parser *pParser, const char *pData, size_t DataSize);
	void ServerListServerCallback(const char *pServerAddr);
};

#endif
