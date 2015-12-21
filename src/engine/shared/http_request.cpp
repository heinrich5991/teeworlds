/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include "http_request.h"

#include <engine/shared/config.h>
#include <game/version.h>

CHttpRequest::CHttpRequest()
{
	m_State = STATE_INACTIVE;
	m_Response.m_pBuffer = 0;

	http_parser_settings_init(&m_HttpParserSettings);
	m_HttpParserSettings.on_body = CHttpRequest::ReceiveCallback;
	m_HttpParserSettings.on_status = CHttpRequest::StatusCallback;
}

CHttpRequest::~CHttpRequest()
{
	mem_free(m_Response.m_pBuffer);
}

void CHttpRequest::Init(int MaxResponseSize)
{
	mem_free(m_Response.m_pBuffer);
	// 1 more for null termination.
	m_Response.m_pBuffer = (char *)mem_alloc(MaxResponseSize + 1, 1);
	m_Response.m_BufferSize = MaxResponseSize;
}

bool CHttpRequest::SocketClosed(int State)
{
	switch(State)
	{
	case STATE_INACTIVE:
	case STATE_ERROR:
	case STATE_SUCCESS:
		return true;
	default:
		return false;
	}
}

void CHttpRequest::ChangeState(int NewState)
{
	if(!SocketClosed(m_State) && SocketClosed(NewState))
	{
		net_tcp_close(m_Socket);
	}
	m_State = NewState;
}

CHttpRequest::CResult CHttpRequest::Result()
{
	CResult Result = { 0 };
	if(m_State == STATE_SUCCESS && m_Status == 200)
	{
		Result.m_pData = m_Response.m_pBuffer;
		Result.m_DataSize = m_Response.m_Size;
	}
	return Result;
}

void CHttpRequest::Request(NETADDR *pAddr, const char *pHost, const char *pUrl)
{
	static const char aFormat[] =
		"GET /teeworlds/%s HTTP/1.1\r\n"
		"Host: %s\r\n"
		"User-Agent: Teeworlds/" GAME_VERSION "\r\n"
		"Connection: close\r\n"
		"\r\n";

	Reset();
	str_format(m_aRequest, sizeof(m_aRequest), aFormat, pUrl, pHost);

	// Don't include null termination in request.
	m_RequestSize = str_length(m_aRequest);
	m_RequestSentSize = 0;

	NETADDR BindAddr = { 0 };
	BindAddr.type = NETTYPE_ALL;
	m_Socket = net_tcp_create(BindAddr);
	net_set_non_blocking(m_Socket);

	net_tcp_connect(&m_Socket, pAddr);

	ChangeState(STATE_CONNECTING);
}

void CHttpRequest::Update()
{
	switch(m_State)
	{
	case STATE_INACTIVE:
	case STATE_SUCCESS:
	case STATE_ERROR:
		return;
	case STATE_CONNECTING:
		if(!net_socket_writable(m_Socket))
		{
			break;
		}
		if(net_socket_error(m_Socket))
		{
			// Connection failed.
			ChangeState(STATE_ERROR);
			break;
		}
		ChangeState(STATE_SENDING_REQUEST);
		// FALLTHROUGH!
	case STATE_SENDING_REQUEST:
	{
		int AlreadySent = m_RequestSentSize;
		char *pRequest = m_aRequest + AlreadySent;
		int Size = m_RequestSize - AlreadySent;
		int Sent = net_tcp_send(m_Socket, pRequest, Size);
		if(Sent == -1)
		{
			if(!net_would_block())
			{
				// Connection failed.
				ChangeState(STATE_ERROR);
			}
			break;
		}
		m_RequestSentSize += Sent;
		if(m_RequestSentSize == m_RequestSize)
		{
			ChangeState(STATE_AWAITING_RESPONSE);
			m_Response.m_Size = 0;
			http_parser_init(&m_Parser, HTTP_RESPONSE);
		}
		break;
	}
	case STATE_AWAITING_RESPONSE:
	{
		char aBuf[4096];
		int Recv = net_tcp_recv(m_Socket, aBuf, sizeof(aBuf));
		if(Recv < 0)
		{
			if(!net_would_block())
			{
				// Connection failed.
				ChangeState(STATE_ERROR);
			}
			break;
		}

		CCallbackData CallbackData;
		CallbackData.m_pResponse = &m_Response;
		CallbackData.m_pStatus = &m_Status;
		m_Parser.data = &CallbackData;
		http_parser_execute(&m_Parser, &m_HttpParserSettings, aBuf, Recv);
		if(m_Parser.http_errno != 0)
		{
			ChangeState(STATE_ERROR);
			break;
		}

		if(Recv == 0)
		{
			// This is safe, we allocated one byte more than we
			// needed.
			m_Response.m_pBuffer[m_Response.m_Size] = 0;
			net_tcp_close(m_Socket);
			ChangeState(STATE_SUCCESS);
			break;
		}
		break;
	}
	}
}

int CHttpRequest::StatusCallback(http_parser *pParser, const char *pData, size_t DataSize)
{
	int *pStatus = ((CCallbackData *)pParser->data)->m_pStatus;
	*pStatus = pParser->status_code;
	return 0;
}

int CHttpRequest::ReceiveCallback(http_parser *pParser, const char *pData, size_t DataSize)
{
	CResponse *pResponse = ((CCallbackData *)pParser->data)->m_pResponse;

	if(pResponse->m_BufferSize - pResponse->m_Size < DataSize)
	{
		pResponse->m_Error = 1;
		return 1;
	}

	mem_copy(pResponse->m_pBuffer + pResponse->m_Size, pData, DataSize);
	pResponse->m_Size += DataSize;

	return 0;
}
