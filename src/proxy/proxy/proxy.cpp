
#include <base/system.h>

#include "proxy_null.h"

#include "0.5-0.6/proxy.h"

#include "proxy.h"

IProxy *CreateProxy(int ServerVer, int ClientVer)
{
	dbg_assert(0 <= ServerVer && ServerVer < NUM_VERSIONS, "invalid server version");
	dbg_assert(0 <= ClientVer && ClientVer < NUM_VERSIONS, "invalid client version");

	if(ClientVer == ServerVer)
		return CreateProxyNull();
	else if(ClientVer == VERSION_05 && ServerVer == VERSION_06)
		return CreateProxy_Client05_Server06();
	else if(ClientVer == VERSION_06 && ServerVer == VERSION_05)
		return CreateProxy_Client06_Server05();

	dbg_assert(false, "invalid version pair");

	return 0;
}

