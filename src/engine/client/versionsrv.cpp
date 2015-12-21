/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include "versionsrv.h"

#include <engine/external/json-parser/json.h>
#include <game/version.h>

CVersionSrv::CVersionSrv()
{
	m_HttpRequest.Init(4 * 1024);
}

void CVersionSrv::Request(NETADDR *pAddr, char *pHostname)
{
	m_HttpRequest.Request(pAddr, pHostname, "/teeworlds/version");
}

void CVersionSrv::Update()
{
	m_HttpRequest.Update();
}

bool CVersionSrv::Done()
{
	return m_HttpRequest.Done();
}

int CVersionSrv::State()
{
	CHttpRequest::CResult Result = m_HttpRequest.Result();
	if(!Result.m_pData)
	{
		return VERSION_ERROR;
	}

	json_value *pJson = json_parse(Result.m_pData);
	if(!pJson)
	{
		return VERSION_ERROR;
	}
	if(pJson->type != json_array)
	{
		json_value_free(pJson);
		return VERSION_ERROR;
	}
	for(int i = 0; i < pJson->u.array.length; i++)
	{
		const json_value &VersionStringJson = (*pJson)[i];
		if(VersionStringJson.type != json_string)
		{
			continue;
		}
		const char *pVersionString = VersionStringJson;
		if(str_comp(pVersionString, GAME_VERSION) == 0)
		{
			return VERSION_UPTODATE;
		}
	}
	json_value_free(pJson);
	return VERSION_OUTOFDATE;
}

void CVersionSrv::Reset()
{
	m_HttpRequest.Reset();
}
