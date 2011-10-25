
#include <base/system.h>

#include <engine/shared/serverbrowser.h>

int main(int argc, const char **argv)
{
	dbg_logger_stdout();
	net_init();

	CStatsServerBrowser Browser;
	for(int i = 0; i < 4; i++)
	{
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "master%d.teeworlds.com", i);
		Browser.AddMaster(aBuf);
	}

	dbg_msg("serverbrowser", "started");

	Browser.Refresh();
	while(Browser.Update())
		thread_sleep(1);

	int NumServers = Browser.NumReceivedServers();
	for(int i = 0; i < NumServers; i++)
	{
		const CServerInfo *pServerInfo = Browser.ServerInfo(i);
		dbg_msg("server", "%s: %s (\"%s\")", Browser.MasterServerHostname(pServerInfo->m_MasterServer), pServerInfo->m_aAddress, pServerInfo->m_aName);
	}

	return 0;
}
