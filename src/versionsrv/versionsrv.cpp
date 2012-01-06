/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>

#include <engine/shared/network.h>
#include <game/version.h>

#include "versionsrv.h"

static int m_NumPackets = 0;

static CNetClient g_NetOp; // main

void SendVerOld(NETADDR *pAddr)
{
	CNetChunk p;
	unsigned char aData[sizeof(VERSIONSRV_LUAVERSION) + sizeof(GAME_LUA_VERSION)];

	mem_copy(aData, VERSIONSRV_LUAVERSION, sizeof(VERSIONSRV_LUAVERSION));
	mem_copy(aData + sizeof(VERSIONSRV_LUAVERSION), GAME_LUA_VERSION, sizeof(GAME_LUA_VERSION));

	p.m_ClientID = -1;
	p.m_Address = *pAddr;
	p.m_Flags = NETSENDFLAG_CONNLESS;
	p.m_pData = aData;
	p.m_DataSize = sizeof(aData);

	g_NetOp.Send(&p);
}

void SendVer(CNetChunk *pPacket)
{
	CNetChunk p;
	unsigned char aData[sizeof(VERSIONSRV_LUAVERSION) + sizeof("1") + sizeof(GAME_LUA_VERSION)];

    char aVersion[sizeof(GAME_LUA_VERSION)];
    char aVersionHash[sizeof(GAME_LUA_VERSION_HASH)];
    mem_copy(aVersion, (unsigned char *)pPacket->m_pData + sizeof(VERSIONSRV_CHECKLUAVERSION), sizeof(aVersion));
    mem_copy(aVersionHash, (unsigned char *)pPacket->m_pData + sizeof(VERSIONSRV_CHECKLUAVERSION) + sizeof(GAME_LUA_VERSION), sizeof(aVersionHash));

	mem_copy(aData, VERSIONSRV_LUAVERSION, sizeof(VERSIONSRV_LUAVERSION));
	bool Match = false;
	if (mem_comp((unsigned char *)pPacket->m_pData + sizeof(VERSIONSRV_CHECKLUAVERSION), GAME_LUA_VERSION, sizeof(GAME_LUA_VERSION)) == 0)
	{
        mem_copy(aData + sizeof(VERSIONSRV_LUAVERSION), "1", sizeof("1"));
	    Match = true;
	}
    else
        mem_copy(aData + sizeof(VERSIONSRV_LUAVERSION), "0", sizeof("0"));
    mem_copy(aData + sizeof(VERSIONSRV_LUAVERSION) + sizeof("1"), GAME_LUA_VERSION, sizeof(GAME_LUA_VERSION));

    dbg_msg("Ver", "%d.%d.%d.%d %s %s %s", pPacket->m_Address.ip[0], pPacket->m_Address.ip[1], pPacket->m_Address.ip[2], pPacket->m_Address.ip[3], aVersion, aVersionHash, Match ? "Match" : "Mismatch");

	p.m_ClientID = -1;
	p.m_Address = pPacket->m_Address;
	p.m_Flags = NETSENDFLAG_CONNLESS;
	p.m_pData = aData;
	p.m_DataSize = sizeof(aData);

	g_NetOp.Send(&p);
}

int main(int argc, char **argv) // ignore_convention
{
	NETADDR BindAddr;

	dbg_logger_stdout();
	net_init();

	mem_zero(&BindAddr, sizeof(BindAddr));
	BindAddr.type = NETTYPE_ALL;
	BindAddr.port = VERSIONSRVLUA_PORT;
	if(!g_NetOp.Open(BindAddr, 0))
	{
		dbg_msg("versionsrv", "couldn't start network");
		return -1;
	}

	dbg_msg("versionsrv", "started");

	while(1)
	{
		g_NetOp.Update();

		// process packets
		CNetChunk Packet;
		while(g_NetOp.Recv(&Packet))
		{

			if(Packet.m_DataSize == sizeof(VERSIONSRV_GETLUAVERSION) && mem_comp(Packet.m_pData, VERSIONSRV_GETLUAVERSION, sizeof(VERSIONSRV_GETLUAVERSION)) == 0)
			{
			    dbg_msg("From", "%d.%d.%d.%d", Packet.m_Address.ip[0], Packet.m_Address.ip[1], Packet.m_Address.ip[2], Packet.m_Address.ip[3]);
				SendVerOld(&Packet.m_Address);
			}
			else if(Packet.m_DataSize == sizeof(VERSIONSRV_CHECKLUAVERSION) + sizeof(GAME_LUA_VERSION) + sizeof(GAME_LUA_VERSION_HASH) && mem_comp(Packet.m_pData, VERSIONSRV_CHECKLUAVERSION, sizeof(VERSIONSRV_CHECKLUAVERSION)) == 0)
			{
				SendVer(&Packet);
			}
			else
			{
			    dbg_msg("Malformed", "%d.%d.%d.%d", Packet.m_Address.ip[0], Packet.m_Address.ip[1], Packet.m_Address.ip[2], Packet.m_Address.ip[3]);
			}
		}

		// be nice to the CPU
		thread_sleep(1);
	}

	return 0;
}
