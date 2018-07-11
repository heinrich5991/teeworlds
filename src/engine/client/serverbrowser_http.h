#ifndef ENGINE_CLIENT_SERVERBROWSER_HTTP_H
#define ENGINE_CLIENT_SERVERBROWSER_HTTP_H
#include <base/system.h>

class CServerInfo;
class IEngine;

class IServerBrowserHttp
{
public:
	virtual ~IServerBrowserHttp() {}

	virtual void Update() = 0;

	virtual bool IsRefreshing() = 0;
	virtual void Refresh() = 0;

	virtual int NumServers() const = 0;
	virtual const NETADDR &ServerAddress(int Index) const = 0;
	virtual void Server(int Index, NETADDR *pAddr, CServerInfo *pInfo) const = 0;
	virtual int NumLegacyServers() const = 0;
	virtual const NETADDR &LegacyServer(int Index) const = 0;
};

IServerBrowserHttp *CreateServerBrowserHttp(IEngine *pEngine);
#endif // ENGINE_CLIENT_SERVERBROWSER_HTTP_H
