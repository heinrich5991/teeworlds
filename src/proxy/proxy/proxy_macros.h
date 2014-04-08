#ifndef PROXY_PROXY_MACROS_H
#define PROXY_PROXY_MACROS_H

#define PROXY_SERVER_PACKET(p) (ServerCB.m_pfnCallback(p, ServerCB.m_pUserdata))
#define PROXY_CLIENT_PACKET(p) (ClientCB.m_pfnCallback(p, ClientCB.m_pUserdata))

#endif // PROXY_PROXY_MACROS_H
