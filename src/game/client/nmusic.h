/* (c) MAP94. See www.n-lvl.com/ndc/nclient/ for more information. */
#ifndef GAME_CLIENT_NMP3_H
#define GAME_CLIENT_NMP3_H

#include "gameclient.h"
#include <base/system.h>
#include <base/tl/sorted_array.h>

class CMusic_DoubleLinkedList
{
public:
    CMusic_DoubleLinkedList *prv;
    CMusic_DoubleLinkedList *nxt;
    char path[1024];
};

class CMusic_Files
{
public:
    char m_aFilename[1024];
    char m_aName[256];
    bool operator<(const CMusic_Files &Other) { return str_comp(m_aName, Other.m_aName) < 0; }
};

class CMusic
{
    class CGameClient *m_pClient;
public:
    CMusic(CGameClient *pClient);
    ~CMusic();
    void Tick();
    static int MusicFetchCallback(const char *pName, int IsDir, int DirType, void *pUser);

    CMusic_DoubleLinkedList *m_NowPlaylist;
    CMusic_DoubleLinkedList *m_NowFiles;
    bool m_Shuffle;
    bool m_Playlist;

    bool m_MusicListActivated;
    bool m_MusicFirstPlay;


    sorted_array<CMusic_Files> m_Files;
};
#endif
