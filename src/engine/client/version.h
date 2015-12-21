/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_CLIENT_VERSION_H
#define ENGINE_CLIENT_VERSION_H

#include <engine/shared/http_request.h>

class CVersion
{
public:
	enum
	{
		VERSION_OUTOFDATE,
		VERSION_UPTODATE,
		VERSION_ERROR,
	};
	CVersion();
	void Request(NETADDR *pAddr, char *pHostname);
	void Update();
	bool Done();
	int State();
	void Reset();

private:
	class CHttpRequest m_HttpRequest;
};

#endif // ENGINE_CLIENT_VERSION_H
