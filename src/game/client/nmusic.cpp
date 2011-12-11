/* (c) MAP94. See www.n-lvl.com/ndc/nclient/ for more information. */
#include "nmusic.h"
#include <engine/shared/config.h>
#include <engine/storage.h>
#include <engine/sound.h>

struct FETCH_CALLBACKINFO
{
    CMusic *m_pSelf;
    const char *m_pPrefix;
};


CMusic::CMusic(CGameClient *pClient)
{
    char Path[1024];
    //m_aDatadir
    if (g_Config.m_ClMusicPath[0])
    {
        str_copy(Path, g_Config.m_ClMusicPath, sizeof(Path));
    }
    else
    {
        char aDataDir[1024];
        char aTemp[1024] = {0};
        int Pos = 0;
        //pClient->Storage()->GetDataDirectory(aDataDir, sizeof(aDataDir));

        for (int i = 0; aDataDir[i]; i++)
            if (aDataDir[i] == '/')
                Pos = i;
        if (Pos < (signed int)sizeof(aTemp))
            str_copy(aTemp, aDataDir, Pos);
        if (Pos)
            str_format(Path, sizeof(Path), "%s/music", aTemp);
        else
            str_copy(Path, "music", sizeof(Path)); //PWD
    }
    m_MusicListActivated = false;
    m_MusicFirstPlay = true;
    m_pClient = pClient;
    FETCH_CALLBACKINFO Info = {this, Path};
	fs_listdir(Path, MusicFetchCallback, 1, &Info);
	Tick();
}

CMusic::~CMusic()
{

}

int CMusic::MusicFetchCallback(const char *pName, int IsDir, int DirType, void *pUser)
{
	if(pName[0] == '.')
		return 0;

	FETCH_CALLBACKINFO *pInfo = (FETCH_CALLBACKINFO *)pUser;

    int len = str_length(pName);
    if (!IsDir && pName[len - 4] == '.' && (pName[len - 3] == 'W' || pName[len - 3] == 'w') && (pName[len - 2] == 'A' || pName[len - 2] == 'a') && (pName[len - 1] == 'V' || pName[len - 1] == 'v'))
    {
        CMusic_Files Item;
        str_format(Item.m_aFilename, sizeof(Item.m_aFilename), "%s/%s", pInfo->m_pPrefix, pName);
        str_copy(Item.m_aName, pName, sizeof(Item.m_aName));
        pInfo->m_pSelf->m_Files.add_unsorted(Item);
    }
	pInfo->m_pSelf->m_Files.sort_range();
    return 0;
}

void CMusic::Tick()
{
    static int MusicOldPlayIndex = -2;
    if (MusicOldPlayIndex == -2) // started - init
    {
        m_pClient->Sound()->m_MusicPlayIndex = 0;
        MusicOldPlayIndex = 0;
        str_copy(m_pClient->Sound()->m_MusicFileName, m_Files[m_pClient->Sound()->m_MusicPlayIndex].m_aFilename, sizeof(m_pClient->Sound()->m_MusicFileName));
    }
    if (m_pClient->Sound()->m_MusicPlayIndex >= m_Files.size())
	{
        m_pClient->Sound()->m_MusicPlayIndex = 0;
        if (m_pClient->Sound()->m_MusicPlaying)
            m_MusicListActivated = true; //force Playing if we Played before
	}
	if (m_pClient->Sound()->m_MusicPlayIndex < 0)
	{
        m_pClient->Sound()->m_MusicPlayIndex = m_Files.size() - 1;
        if (m_pClient->Sound()->m_MusicPlaying)
            m_MusicListActivated = true; //force Playing if we Played before
	}
	if (m_MusicListActivated || m_pClient->Sound()->m_MusicPlayIndex != MusicOldPlayIndex)
	{
        str_copy(m_pClient->Sound()->m_MusicFileName, m_Files[m_pClient->Sound()->m_MusicPlayIndex].m_aFilename, sizeof(m_pClient->Sound()->m_MusicFileName));
	    m_pClient->Sound()->m_MusicLoadNew = true;
	    m_MusicListActivated = false;
	    m_MusicFirstPlay = false;

        CNMsg::CPopupMsg Msg;
        str_copy(Msg.m_aTitle, Localize("Music"), sizeof(Msg.m_aTitle));
        str_copy(Msg.m_aText, m_Files[m_pClient->Sound()->m_MusicPlayIndex].m_aFilename, sizeof(Msg.m_aText));
        m_pClient->m_pLua->m_EventListener.OnEvent("OnMusicChange");
        Msg.m_Show = true;
        Msg.m_Time = 1;
        Msg.m_Size = CNMsg::CPopupMsg::POPUPSIZEDEFAULT;
        m_pClient->m_Msgs->AddMsg(Msg);
    }
	MusicOldPlayIndex = m_pClient->Sound()->m_MusicPlayIndex;
}
