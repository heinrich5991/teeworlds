/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_CLIENT_VERSIONSRV_H
#define ENGINE_CLIENT_VERSIONSRV_H

#include <engine/shared/http_request.h>

class CVersionSrv
{
public:
	enum
	{
		VERSION_OUTOFDATE,
		VERSION_UPTODATE,
		VERSION_ERROR,
	};
	CVersionSrv();
	void Request(NETADDR *pAddr, char *pHostname);
	void Update();
	bool Done();
	int State();
	void Reset();

private:
	class CHttpRequest m_HttpRequest;
};

#endif // ENGINE_CLIENT_VERSIONSRV_H
