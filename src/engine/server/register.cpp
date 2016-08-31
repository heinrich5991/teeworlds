/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <engine/console.h>
#include <engine/external/json-parser/json.h>
#include <engine/masterserver.h>
#include <engine/shared/config.h>
#include <engine/shared/network.h>
#include <engine/shared/packer.h>
#include <game/version.h>

#include "register.h"

static const unsigned char SERVERBROWSE_FWCHECK[] = {255, 255, 255, 255, 'f', 'w', '?', '2'};
CRegister::CRegister()
{
	m_pNetServer = 0;
	m_pMasterServer = 0;
	m_pConsole = 0;

	m_RegisterState = REGISTERSTATE_START;
	m_RegisterStateStart = 0;
	m_RegisterFirst = 1;
	m_RegisterCount = 0;
	m_GotHeartbeatResponse = 0;

	m_FwCheckDone = true;
	m_FwCheckTime = 0;
	m_FwCheckAlternatePort = true;
	m_NumFwChecks = 0;
	m_FwPort = 0;
	mem_zero(m_aFwCheckToken, sizeof(m_aFwCheckToken));

	for(int i = 0; i < IMasterServer::MAX_MASTERSERVERS; i++)
	{
		mem_zero(&m_aMasterservers[i].m_Addr, sizeof(m_aMasterservers[i].m_Addr));
		m_aMasterservers[i].m_Count = 0;
		m_aMasterservers[i].m_Valid = 0;
		m_aMasterservers[i].m_Blacklisted = 0;
		m_aMasterservers[i].m_HttpRequest.Init(4 * 1024);
	}
	m_RegisterRegisteredServer = -1;
}

void CRegister::RegisterNewState(int State)
{
	m_RegisterState = State;
	m_RegisterStateStart = time_get();
}

void CRegister::BlacklistMaster(int i, int Seconds)
{
	m_aMasterservers[i].m_Blacklisted = time_get() + time_freq() * Seconds;
	m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "register", "WARNING: Master server is not responding properly, switching master");
	RegisterNewState(REGISTERSTATE_START);
}

void CRegister::RegisterSendFwCheck(int i)
{
	int Port = g_Config.m_SvPort;
	if(g_Config.m_SvExternalPort)
	{
		if(m_FwCheckAlternatePort)
		{
			Port = g_Config.m_SvExternalPort;
		}
		m_FwCheckAlternatePort = !m_FwCheckAlternatePort;
	}
	char aBuffer[64];
	str_format(aBuffer, sizeof(aBuffer),
		"{\"port\":%d,\"token\":\"%s\"}",
		Port, m_aFwCheckToken);
	m_aMasterservers[i].m_HttpRequest.PostJson(
		&m_aMasterservers[i].m_Addr,
		m_pMasterServer->GetName(i),
		HTTP_VERSION "/dynamic/fwcheck",
		aBuffer
	);
	m_FwCheckTime = time_get();
	m_FwCheckDone = false;
}

void CRegister::RegisterSendHeartbeat(int i)
{
	char aBuffer[128];
	if(m_FwPort)
	{
		str_format(aBuffer, sizeof(aBuffer),
			"{\"port\":%d,\"fwtoken\":\"%s\"}\n",
			m_FwPort, m_aFwCheckResult);
	}
	else
	{
		int Port = g_Config.m_SvPort;
		if(g_Config.m_SvExternalPort)
			Port = g_Config.m_SvExternalPort;
		str_format(aBuffer, sizeof(aBuffer), "{\"port\":%d}", Port);
	}
	m_aMasterservers[i].m_HttpRequest.PostJson(
		&m_aMasterservers[i].m_Addr,
		m_pMasterServer->GetName(i),
		HTTP_VERSION "/dynamic/register",
		aBuffer
	);
	m_GotHeartbeatResponse = 0;
}

void CRegister::RegisterGotHeartbeatResponse(int i, char *pData)
{
	json_value *pJson = json_parse(pData);
	if(!pJson)
	{
		dbg_msg("register", "invalid json (1): %s", pData);
		BlacklistMaster(i);
		return;
	}

	const json_value &ResultJson = (*pJson)["result"];
	const json_value &MessageJson = (*pJson)["message"];
	if(ResultJson.type != json_string ||
		(MessageJson.type != json_string && MessageJson.type != json_none))
	{
		dbg_msg("register", "invalid json (2): %s", pData);
		BlacklistMaster(i);
		json_value_free(pJson);
		return;
	}

	if(str_comp(ResultJson, "fwcheck") == 0)
	{
		m_FwCheckAlternatePort = true;
		m_NumFwChecks = 0;
		// TODO!
		str_copy(m_aFwCheckToken, "foobar", sizeof(m_aFwCheckToken));
		RegisterNewState(REGISTERSTATE_FWCHECK);
		json_value_free(pJson);
		return;
	}
	else if(str_comp(ResultJson, "error") == 0)
	{
		if(MessageJson.type == json_string)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "ERROR: %s", (const char *)MessageJson);
			m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "register", aBuf);
		}
		else
		{
			m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "register", "ERROR registering");
		}
		BlacklistMaster(i, 60);
		json_value_free(pJson);
		return;
	}
	else if(str_comp(ResultJson, "ok") == 0)
	{
		if(MessageJson.type == json_string)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "INFO: %s", (const char *)MessageJson);
			m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "register", aBuf);
		}
		// Fall through.
	}
	else
	{
		dbg_msg("register", "invalid json (3): %s", pData);
		BlacklistMaster(i);
		json_value_free(pJson);
		return;
	}
	json_value_free(pJson);

	m_GotHeartbeatResponse = 1;
	if(m_RegisterFirst)
	{
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "register", "server registered");
		m_RegisterFirst = 0;
	}

	// Redo the whole process after 60 minutes to balance out the master servers.
	if(m_RegisterCount == 240)
	{
		RegisterNewState(REGISTERSTATE_START);
	}
	else
	{
		m_RegisterCount++;
		RegisterNewState(REGISTERSTATE_HEARTBEAT);
		m_FwPort = 0;
	}
}

void CRegister::RegisterSendCountRequest(int i)
{
	m_aMasterservers[i].m_HttpRequest.Request(
		&m_aMasterservers[i].m_Addr,
		m_pMasterServer->GetName(i),
		HTTP_VERSION "/dynamic/info"
	);
}

void CRegister::RegisterGotCount(int i, char *pData)
{
	json_value *pJson = json_parse(pData);
	if(!pJson)
	{
		m_aMasterservers[i].m_Valid = 0;
		return;
	}
	const json_value &CountJson = (*pJson)["count"];
	m_aMasterservers[i].m_Valid = 0;
	if(CountJson.type == json_integer)
	{
		m_aMasterservers[i].m_Count = CountJson.u.integer;
		m_aMasterservers[i].m_Valid = 1;
	}
	json_value_free(pJson);
}

void CRegister::Init(CNetServer *pNetServer, IEngineMasterServer *pMasterServer, IConsole *pConsole)
{
	m_pNetServer = pNetServer;
	m_pMasterServer = pMasterServer;
	m_pConsole = pConsole;
}

void CRegister::RegisterUpdate(int Nettype)
{
	int64 Now = time_get();
	int64 Freq = time_freq();

	if(!g_Config.m_SvRegister)
		return;

	m_pMasterServer->Update();

	for(int i = 0; i < IMasterServer::MAX_MASTERSERVERS; i++)
	{
		m_aMasterservers[i].m_HttpRequest.Update();
	}

	if(m_RegisterState == REGISTERSTATE_START)
	{
		m_RegisterCount = 0;
		m_RegisterFirst = 1;
		RegisterNewState(REGISTERSTATE_UPDATE_ADDRS);
		m_pMasterServer->RefreshAddresses(Nettype);
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "register", "refreshing ip addresses");
	}
	else if(m_RegisterState == REGISTERSTATE_UPDATE_ADDRS)
	{
		m_RegisterRegisteredServer = -1;

		if(!m_pMasterServer->IsRefreshing())
		{
			int i;
			for(i = 0; i < IMasterServer::MAX_MASTERSERVERS; i++)
			{
				if(!m_pMasterServer->IsValid(i)
					|| Now <= m_aMasterservers[i].m_Blacklisted)
				{
					m_aMasterservers[i].m_Valid = 0;
					m_aMasterservers[i].m_Count = 0;
					continue;
				}

				NETADDR Addr = m_pMasterServer->GetAddr(i);
				m_aMasterservers[i].m_Addr = Addr;
				m_aMasterservers[i].m_Valid = 1;
				m_aMasterservers[i].m_Count = -1;
				RegisterSendCountRequest(i);
			}

			m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "register", "fetching server counts");
			RegisterNewState(REGISTERSTATE_QUERY_COUNT);
		}
	}
	else if(m_RegisterState == REGISTERSTATE_QUERY_COUNT)
	{
		int Left = 0;
		for(int i = 0; i < IMasterServer::MAX_MASTERSERVERS; i++)
		{
			if(!m_aMasterservers[i].m_Valid)
				continue;

			if(m_aMasterservers[i].m_Count != -1)
				continue;

			if(!m_aMasterservers[i].m_HttpRequest.Done())
			{
				Left++;
				continue;
			}

			CHttpRequest::CResult Result = m_aMasterservers[i].m_HttpRequest.Result();
			if(!Result.m_pData)
			{
				m_aMasterservers[i].m_Valid = 0;
				continue;
			}

			RegisterGotCount(i, Result.m_pData);

			m_aMasterservers[i].m_HttpRequest.Reset();
		}

		// check if we are done or timed out
		if(Left == 0 || Now > m_RegisterStateStart+Freq*3)
		{
			// choose server
			int Best = -1;
			int i;
			for(i = 0; i < IMasterServer::MAX_MASTERSERVERS; i++)
			{
				if(!m_aMasterservers[i].m_Valid || m_aMasterservers[i].m_Count == -1)
					continue;

				if(Best == -1 || m_aMasterservers[i].m_Count < m_aMasterservers[Best].m_Count)
					Best = i;
			}

			// server chosen
			m_RegisterRegisteredServer = Best;
			if(m_RegisterRegisteredServer == -1)
			{
				m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "register", "WARNING: No master servers. Retrying in 60 seconds");
				RegisterNewState(REGISTERSTATE_ERROR);
			}
			else
			{
				char aBuf[256];
				str_format(aBuf, sizeof(aBuf), "chose '%s' as master, sending heartbeats", m_pMasterServer->GetName(m_RegisterRegisteredServer));
				m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "register", aBuf);
				RegisterNewState(REGISTERSTATE_HEARTBEAT);
				m_FwPort = 0;
				RegisterSendHeartbeat(m_RegisterRegisteredServer);
			}
		}
	}
	else if(m_RegisterState == REGISTERSTATE_FWCHECK)
	{
		int i = m_RegisterRegisteredServer;
		if(m_aMasterservers[i].m_HttpRequest.Done())
		{
			CHttpRequest::CResult Result = m_aMasterservers[i].m_HttpRequest.Result();
			if(Result.m_pData)
			{
				m_NumFwChecks++;
				m_FwCheckDone = true;
			}
			else
			{
				dbg_msg("register", "could not POST fwcheck request");
				BlacklistMaster(i);
			}
			m_aMasterservers[i].m_HttpRequest.Reset();
		}
		if(Now > m_FwCheckTime + Freq && m_FwCheckDone)
		{
			RegisterSendFwCheck(i);
		}
		if(Now > m_RegisterStateStart + Freq * 60 || m_NumFwChecks >= 10)
		{
			if(m_NumFwChecks < 5)
			{
				dbg_msg("register", "master server slow to respond to fwcheck requests");
				BlacklistMaster(i);
			}
			else
			{
				char aBuf[256];
				m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "register", "ERROR: the master server reports that clients cannot connect to this server.");
				str_format(aBuf, sizeof(aBuf), "ERROR: configure your firewall/nat to let through udp on port %d.", g_Config.m_SvPort);
				m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "register", aBuf);
				RegisterNewState(REGISTERSTATE_ERROR);
			}
		}
	}
	else if(m_RegisterState == REGISTERSTATE_HEARTBEAT)
	{
		int i = m_RegisterRegisteredServer;
		if(m_aMasterservers[i].m_HttpRequest.Done())
		{
			CHttpRequest::CResult Result = m_aMasterservers[i].m_HttpRequest.Result();
			if(Result.m_pData)
			{
				RegisterGotHeartbeatResponse(i, Result.m_pData);
			}
			else
			{
				dbg_msg("register", "could not POST register request");
				BlacklistMaster(i);
			}
			m_aMasterservers[i].m_HttpRequest.Reset();
			if(m_RegisterState == REGISTERSTATE_FWCHECK)
			{
				RegisterSendFwCheck(i);
			}
		}
		else if(Now > m_RegisterStateStart+Freq*15)
		{
			if(!m_GotHeartbeatResponse)
			{
				dbg_msg("register", "no heartbeat answer in 15s");
				BlacklistMaster(i);
			}
			else
			{
				RegisterNewState(REGISTERSTATE_HEARTBEAT);
				RegisterSendHeartbeat(i);
			}
		}
	}
	else if(m_RegisterState == REGISTERSTATE_ERROR)
	{
		// check for restart
		if(Now > m_RegisterStateStart+Freq*60)
		{
			for(int i = 0; i < IMasterServer::MAX_MASTERSERVERS; i++)
			{
				m_aMasterservers[i].m_Blacklisted = 0;
			}
			RegisterNewState(REGISTERSTATE_START);
		}
	}
}

int CRegister::RegisterProcessPacket(CNetChunk *pPacket, TOKEN Token)
{
	if(m_RegisterState == REGISTERSTATE_FWCHECK)
	{
		if(pPacket->m_DataSize >= sizeof(SERVERBROWSE_FWCHECK)
			&& mem_comp(pPacket->m_pData, SERVERBROWSE_FWCHECK, sizeof(SERVERBROWSE_FWCHECK)) == 0)
		{
			CUnpacker Up;
			char *pData = ((char *)pPacket->m_pData) + sizeof(SERVERBROWSE_FWCHECK);
			Up.Reset(pData, pPacket->m_DataSize - sizeof(SERVERBROWSE_FWCHECK));
			const char *pToken = Up.GetString();
			const char *pPort = Up.GetString();
			const char *pFwToken = Up.GetString();
			if(Up.Error())
			{
				// TODO: Remove these?
				dbg_msg("register", "got invalid fwcheck");
				return 1;
			}
			if(str_comp(pToken, m_aFwCheckToken) != 0)
			{
				dbg_msg("register", "got invalid fwcheck token");
				return 1;
			}
			m_FwPort = str_toint(pPort);
			str_copy(m_aFwCheckResult, pFwToken, sizeof(m_aFwCheckResult));
			RegisterNewState(REGISTERSTATE_HEARTBEAT);
			return 1;
		}
	}
	return 0;
}
