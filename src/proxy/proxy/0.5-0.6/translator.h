#ifndef PROXY_05_06_TRANSLATOR_H
#define PROXY_05_06_TRANSLATOR_H

#include <proxy/proxy/translator.h>

ITranslator *CreateTranslator_05_06(IHacks *pHacks, PACKET_FUNC pfnTranslatePacketCB, void *pUserData);
ITranslator *CreateTranslator_06_05(IHacks *pHacks, PACKET_FUNC pfnTranslatePacketCB, void *pUserData);

#endif // PROXY_05_06_TRANSLATOR_H
