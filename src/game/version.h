/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_VERSION_H
#define GAME_VERSION_H
#include "generated/nethash.cpp"
#include "generated/luahash.cpp"
#define GAME_VERSION "0.6.1"
#define GAME_NETVERSION "0.6 626fce9a778df4d4" //the std game version

#define GAME_NETVERSION_LUA "0.6 " GAME_NETVERSION_HASH
#define GAME_LUA_VERSION_HASH "1.1 " GAME_LUAVERSION_HASH
#define GAME_LUA_VERSION "1.1"

#define GAME_LUA_VERSION_MATCH "1"
#define GAME_LUA_VERSION_NOTMATCH "0"

#endif
