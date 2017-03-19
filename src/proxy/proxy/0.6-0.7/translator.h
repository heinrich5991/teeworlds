#ifndef PROXY_06_07_TRANSLATOR_H
#define PROXY_06_07_TRANSLATOR_H

#include <proxy/proxy/translator.h>

ITranslator *CreateTranslator_06_07(IHacks *pHacks, PACKET_FUNC pfnTranslatePacketCB, void *pUserData);
ITranslator *CreateTranslator_07_06(IHacks *pHacks, PACKET_FUNC pfnTranslatePacketCB, void *pUserData);

#endif // PROXY_06_07_TRANSLATOR_H
