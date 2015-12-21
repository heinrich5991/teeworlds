/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_HTTP_REQUEST_H
#define ENGINE_SHARED_HTTP_REQUEST_H

#include <base/system.h>
#include <engine/external/http_parser/http_parser.h>

class CHttpRequest
{
public:
	struct CResult
	{
		char *m_pData;
		int m_DataSize;
	};

	CHttpRequest();
	~CHttpRequest();
	void Init(int MaxResponseSize);
	void Request(NETADDR *pAddr, const char *pHost, const char *pUrl);
	void Update();
	void Close();

	bool Done() { return m_State == STATE_ERROR || m_State == STATE_SUCCESS; }
	CResult Result();

private:
	void Error();
	static bool SocketClosed(int State);
	void ChangeState(int NewState);

	enum
	{
		STATE_ERROR=-1,
		STATE_INACTIVE=0,
		STATE_RESOLVING,
		STATE_CONNECTING,
		STATE_SENDING_REQUEST,
		STATE_AWAITING_RESPONSE,
		STATE_SUCCESS,
	};
	int m_State;
	NETSOCKET m_Socket;

	char m_aRequest[1024];
	int m_RequestSize;
	int m_RequestSentSize;

	http_parser m_Parser;
	http_parser_settings m_HttpParserSettings;

	struct CResponse
	{
		char *m_pBuffer;
		size_t m_BufferSize;
		size_t m_Size;
		bool m_Error;
	};
	int m_Status;
	CResponse m_Response;

	struct CCallbackData
	{
		CResponse *m_pResponse;
		int *m_pStatus;
	};

	static int StatusCallback(http_parser *pParser, const char *pData, size_t DataSize);
	static int ReceiveCallback(http_parser *pParser, const char *pData, size_t DataSize);
};

#endif // ENGINE_SHARED_HTTP_REQUEST_H
