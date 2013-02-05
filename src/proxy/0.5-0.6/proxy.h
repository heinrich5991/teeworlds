#ifndef PROXY_05ENDOFLINE_PROXY_H
#define PROXY_05ENDOFLINE_PROXY_H

#include <proxy/proxy.h>

IProxy *CreateProxy_05_06(IHacks *pHacks, PACKET_FUNC pfnTranslatePacketCB, void *pUserData);
IProxy *CreateProxy_06_05(IHacks *pHacks, PACKET_FUNC pfnTranslatePacketCB, void *pUserData);

#endif // PROXY_05ENDOFLINE_PROXY_H
