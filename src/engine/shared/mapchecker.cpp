/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>
#include <base/system.h>

#include <engine/external/json-parser/json.h>
#include <engine/storage.h>

#include <game/version.h>

#include <versionsrv/versionsrv.h>
#include <versionsrv/mapversions.h>

#include "mapchecker.h"

CMapChecker::CMapChecker()
{
	m_HttpRequest.Init(4 * 1024);
	Init();
	SetDefaults();
}

void CMapChecker::Init()
{
	m_Whitelist.Reset();
	m_pFirst = 0;
}

void CMapChecker::SetDefaults()
{
	AddMaplist(s_aMapVersionList, s_NumMapVersionItems);
}

void CMapChecker::AddMaplist(CMapVersion *pMaplist, int Num)
{
	for(int i = 0; i < Num; ++i)
	{
		unsigned Crc = (pMaplist[i].m_aCrc[0]<<24) | (pMaplist[i].m_aCrc[1]<<16) | (pMaplist[i].m_aCrc[2]<<8) | pMaplist[i].m_aCrc[3];
		unsigned Size = (pMaplist[i].m_aSize[0]<<24) | (pMaplist[i].m_aSize[1]<<16) | (pMaplist[i].m_aSize[2]<<8) | pMaplist[i].m_aSize[3];
		AddEntry(pMaplist[i].m_aName, Crc, Size);
	}
}

void CMapChecker::AddEntry(const char *pMapName, unsigned MapCrc, unsigned MapSize)
{
	CWhitelistEntry *pEntry = (CWhitelistEntry *)m_Whitelist.Allocate(sizeof(CWhitelistEntry));
	pEntry->m_pNext = m_pFirst;
	m_pFirst = pEntry;

	str_copy(pEntry->m_aMapName, pMapName, sizeof(pEntry->m_aMapName));
	pEntry->m_MapCrc = MapCrc;
	pEntry->m_MapSize = MapSize;
}

bool CMapChecker::IsMapValid(const char *pMapName, unsigned MapCrc, unsigned MapSize)
{
	bool StandardMap = false;
	for(CWhitelistEntry *pCurrent = m_pFirst; pCurrent; pCurrent = pCurrent->m_pNext)
	{
		if(str_comp(pCurrent->m_aMapName, pMapName) == 0)
		{
			StandardMap = true;
			if(pCurrent->m_MapCrc == MapCrc && pCurrent->m_MapSize == MapSize)
				return true;
		}
	}

	return !StandardMap;
}

bool CMapChecker::ReadAndValidateMap(IStorage *pStorage, const char *pFilename, int StorageType)
{
	// extract map name
	char aMapName[MAX_MAP_LENGTH];
	char aMapNameExt[MAX_MAP_LENGTH+4];
	bool StandardMap = false;
	const char *pExtractedName = pFilename;
	const char *pEnd = 0;

	for(const char *pSrc = pFilename; *pSrc; ++pSrc)
	{
		if(*pSrc == '/' || *pSrc == '\\')
			pExtractedName = pSrc+1;
		else if(*pSrc == '.')
			pEnd = pSrc;
	}

	int Length = (int)(pEnd - pExtractedName);
	if(Length <= 0 || Length >= MAX_MAP_LENGTH)
		return true;
	str_copy(aMapName, pExtractedName, min((int)MAX_MAP_LENGTH, (int)(pEnd-pExtractedName+1)));
	str_format(aMapNameExt, sizeof(aMapNameExt), "%s.map", aMapName);

	// check for valid map
	for(CWhitelistEntry *pCurrent = m_pFirst; pCurrent; pCurrent = pCurrent->m_pNext)
	{
		if(str_comp(pCurrent->m_aMapName, aMapName) == 0)
		{
			StandardMap = true;
			char aBuffer[512]; // TODO: MAX_PATH_LENGTH (512) should be defined in a more central header and not in storage.cpp and editor.h
			if(pStorage->FindFile(aMapNameExt, "maps", StorageType, aBuffer, sizeof(aBuffer), pCurrent->m_MapCrc, pCurrent->m_MapSize))
				return true;
		}
	}

	return !StandardMap;
}

void CMapChecker::Request(NETADDR *pAddr, char *pHostname, char *pPath)
{
	m_HttpRequest.Request(pAddr, pHostname, pPath, "/maps");
}

void CMapChecker::Update()
{
	m_HttpRequest.Update();
	if(m_HttpRequest.Done())
	{
		CHttpRequest::CResult Result = m_HttpRequest.Result();
		if(Result.m_pData)
		{
			Init();
			ParseHttpResponse(Result.m_pData);
		}
		m_HttpRequest.Reset();
	}
}

void CMapChecker::ParseHttpResponse(char *pResponse)
{
	json_value *pJson = json_parse(pResponse);
	if(!pJson)
	{
		return;
	}
	if(pJson->type != json_object)
	{
		json_value_free(pJson);
		return;
	}
	for(int i = 0; i < pJson->u.object.length; i++)
	{
		const json_value &Array = *pJson->u.object.values[i].value;
		const char *pName = pJson->u.object.values[i].name;
		if(Array.type != json_array)
		{
			continue;
		}
		for(int j = 0; j < Array.u.array.length; j++)
		{
			const json_value &MapVersion = Array[i];
			const json_value &CrcJson = MapVersion["crc"];
			const json_value &SizeJson = MapVersion["size"];
			if(CrcJson.type != json_integer
				|| SizeJson.type != json_integer)
			{
				continue;
			}
			AddEntry(pName, CrcJson.u.integer, SizeJson.u.integer);
		}
	}
	json_value_free(pJson);
	return;
}
