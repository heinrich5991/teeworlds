
#include <base/math.h>


#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>

#include <game/generated/protocol.h>
#include <game/generated/client_data.h>

#include <game/client/ui.h>
#include <game/client/render.h>
#include <game/client/gameclient.h>
#include <game/client/animstate.h>
#include <game/localization.h>

#include "binds.h"
#include "menus.h"
#include "skins.h"
#include <game/client/nmusic.h>
#include <engine/sound.h>

void CMenus::RenderMusic(CUIRect MainView)
{
	CUIRect List;
    CUIRect Button;
    CUIRect Player;
    CUIRect Slider;
	// render background
	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActive, CUI::CORNER_ALL, 10.0f);

	MainView.Margin(10.0f, &MainView);
	MainView.HSplitTop(30.0f, 0, &MainView);
    MainView.VSplitMid(&MainView, &List);
    List.VSplitLeft(10.0f, 0, &List);
    MainView.VSplitRight(10.0f, &MainView, 0);

    static int MusicFileList = 0;
    static float MusicFileListScrole = 0;

    char Bottom[256];
    str_format(Bottom, sizeof(Bottom), "%i %s", m_pClient->m_Music->m_Files.size(), Localize("files"));
    UiDoListboxStart(&MusicFileList, &List, 17.0f, Localize("Music files"), Bottom, m_pClient->m_Music->m_Files.size(), 1, Sound()->m_MusicPlayIndex, MusicFileListScrole);
	for(sorted_array<CMusic_Files>::range r = m_pClient->m_Music->m_Files.all(); !r.empty(); r.pop_front())
	{
		CListboxItem Item = UiDoListboxNextItem((void*)(&r.front()));
		if(Item.m_Visible)
			UI()->DoLabel(&Item.m_Rect, r.front().m_aName, Item.m_Rect.h*ms_FontmodHeight, -1);
	}
	Sound()->m_MusicPlayIndex = UiDoListboxEnd(&MusicFileListScrole, &m_pClient->m_Music->m_MusicListActivated);


	static int MusicSeekBar = 0;
	void *id = &MusicSeekBar;
	int MusicSecondsPlayed = 0;
	int MusicSecondsTotal = 0;
	if (Sound()->m_MusicWaveFileHeader.b_pro_sec)
	{
        MusicSecondsPlayed = Sound()->m_MusicPlayedBytes / Sound()->m_MusicWaveFileHeader.b_pro_sec;
        MusicSecondsTotal = Sound()->m_MusicWaveFileHeader.riff_laenge / Sound()->m_MusicWaveFileHeader.b_pro_sec;
	}


    MainView.HSplitTop(100.0f, &Player, &MainView);

    RenderTools()->DrawUIRect(&Player, vec4(0,0,0,0.25f), CUI::CORNER_ALL, 5.0f);
    Player.HSplitTop(30.0f, &Button, &Player);
    static int s_PlayPause = 0;
    static int s_Prev = 0;
    static int s_Next = 0;

    Button.x = Button.x + Button.w / 2 - 12.5f;
    Button.w = 25.0f;

    int Icon = 0;
    if (Sound()->m_MusicPlaying)
        Icon = 0;
    else
        Icon = 1;
    if (DoButton_MusicIcon(&s_PlayPause, Icon, &Button))
    {
        Sound()->m_MusicPlaying = Sound()->m_MusicPlaying ^ 1;
        if (m_pClient->m_Music->m_MusicFirstPlay)
            m_pClient->m_Music->m_MusicListActivated = true;
    }
    Button.x = Button.x - 30.0f;
    if (DoButton_MusicIcon(&s_Prev, 2, &Button))
    {
        Sound()->m_MusicPlayIndex--;
    }
    Button.x = Button.x + 60.0f;
    if (DoButton_MusicIcon(&s_Next, 3, &Button))
    {
        Sound()->m_MusicPlayIndex++;
    }


    Player.HSplitTop(20.0f, &Button, &Player);
    Button.HMargin(2.0f, &Button);
    Button.VMargin(5.0f, &Button);
    RenderTools()->DrawUIRect(&Button, vec4(0,0,0,0.5f), CUI::CORNER_ALL, 5.0f);
    CUIRect FilledBar = Button;
    FilledBar.w = 10.0f + (FilledBar.w-10.0f)*(float)MusicSecondsPlayed / (float)MusicSecondsTotal;
    RenderTools()->DrawUIRect(&FilledBar, vec4(1,1,1,0.5f), CUI::CORNER_ALL, 5.0f);
    char aBuf[256];
    str_format(aBuf, sizeof(aBuf), "%i:%s%i/%i:%s%i", (int)MusicSecondsPlayed/60, ((int)MusicSecondsPlayed%60)<10?"0":"", (int)MusicSecondsPlayed%60, (int)MusicSecondsTotal/60, ((int)MusicSecondsTotal%60)<10?"0":"", (int)MusicSecondsTotal%60);

	UI()->DoLabel(&Button, aBuf, 14.0, 0);
    int Inside = UI()->MouseInside(&Button);
    if(UI()->ActiveItem() == id)
    {
        if(!UI()->MouseButton(0))
            UI()->SetActiveItem(0);
        else
        {
            static float PrevAmount = 0.0f;
            float Amount = (UI()->MouseX()-Button.x)/(float)Button.w;
            if(Amount > 0 && Amount < 1.0f && PrevAmount != Amount)
            {
                PrevAmount = Amount;
                if (Sound()->m_MusicFileHandle && MusicSecondsTotal > 1)
                {
                    int SetAmount = Amount * MusicSecondsTotal * Sound()->m_MusicWaveFileHeader.b_pro_sec + Sound()->m_MusicWaveFileHeader.fmt_laenge + 21;
                    if (SetAmount % 2 == 0)
                        SetAmount++;
                    io_seek(Sound()->m_MusicFileHandle, SetAmount, IOSEEK_START);
                    Sound()->m_MusicPlayedBytes = SetAmount;
                    Sound()->m_MusicTmpBufferIn = 0;
                    Sound()->m_MusicTmpBufferWrite = 0;
                    Sound()->m_MusicTmpBufferRead = 0;
                }
            }
        }
    }
    else if(UI()->HotItem() == id)
    {
        if(UI()->MouseButton(0))
            UI()->SetActiveItem(id);
    }

    if(Inside)
        UI()->SetHotItem(id);

    Player.HSplitTop(10.0f, 0, &Player);
    Player.HSplitTop(27.0f, &Slider, &Player);
    Slider.Margin(5.0f, &Slider);
    static int sVolumeId = 0;
    g_Config.m_SndMusicVolume = DoScrollbarH(&sVolumeId, &Slider, (float)g_Config.m_SndMusicVolume / 100.0f) * 100;

    if (m_pClient->m_Music->m_Files.size() == 0)
    {
        Sound()->m_MusicPlaying = false;
        m_pClient->m_Music->m_MusicListActivated = false;
        Sound()->m_MusicPlayIndex = 0;
    }

    CUIRect Controls;
    MainView.HSplitTop(10.0f, 0, &MainView);
    MainView.HSplitTop(130.0f, &Controls, &MainView);
    RenderTools()->DrawUIRect(&Controls, vec4(0, 0, 0, 0.25f), CUI::CORNER_ALL, 5.0f);

    Controls.VMargin(5.0f, &Controls);
    Controls.HSplitTop(20.0f, &Button, &Controls);

    typedef struct
    {
        CLocConstString m_Name;
        const char *m_pCommand;
        int m_KeyId;
    } CKeyInfo;

    static CKeyInfo saKeys[] =
    {
        { "Pause", "music pause", 0},		// Localize - these strings are localized within CLocConstString
        { "Next", "music next", 0 },
        { "Previous", "music prev", 0 },
        { "Louder", "music +", 0 },
        { "Lower", "music -", 0 },
    };

    static int sKeyCount = sizeof(saKeys) / sizeof(CKeyInfo);

    for(int i = 0; i < sKeyCount; i++)
		saKeys[i].m_KeyId = 0;

	for(int KeyId = 0; KeyId < KEY_LAST; KeyId++)
	{
		const char *pBind = m_pClient->m_pBinds->Get(KeyId);
		if(!pBind[0])
			continue;

		for(int i = 0; i < sKeyCount; i++)
			if(str_comp(pBind, saKeys[i].m_pCommand) == 0)
			{
				saKeys[i].m_KeyId = KeyId;
				break;
			}
	}
    static int sMusicPauseId = 0;
    static int sMusicNextId = 0;
    static int sMusicPrevId = 0;
    static int sMusicLouderId = 0;
    static int sMusicLowerId = 0;

    CUIRect Label;
    Button.HSplitTop(5.0f, 0, &Button);
    Button.HSplitTop(20.0f, &Button, 0);
    Button.VSplitLeft(100.0f, &Label, &Button);
    UI()->DoLabel(&Label, saKeys[0].m_Name, 18.0f*ms_FontmodHeight, -1);
    int MusicPauseKey = DoKeyReader(&sMusicPauseId, &Button, saKeys[0].m_KeyId);
    if(MusicPauseKey != saKeys[0].m_KeyId)
    {
        if(saKeys[0].m_KeyId != 0 || MusicPauseKey == 0)
            m_pClient->m_pBinds->Bind(saKeys[0].m_KeyId, "");
        if(MusicPauseKey != 0)
            m_pClient->m_pBinds->Bind(MusicPauseKey, saKeys[0].m_pCommand);
    }

    Button.HSplitTop(25.0f, 0, &Button);
    Button.HSplitTop(20.0f, &Button, 0);
    Label.HSplitTop(25.0f, 0, &Label);
    Label.HSplitTop(20.0f, &Label, 0);
    UI()->DoLabel(&Label, saKeys[1].m_Name, 18.0f*ms_FontmodHeight, -1);
    int MusicNextKey = DoKeyReader(&sMusicNextId, &Button, saKeys[1].m_KeyId);
    if(MusicNextKey != saKeys[1].m_KeyId)
    {
        if(saKeys[1].m_KeyId != 0 || MusicNextKey == 0)
            m_pClient->m_pBinds->Bind(saKeys[1].m_KeyId, "");
        if(MusicNextKey != 0)
            m_pClient->m_pBinds->Bind(MusicNextKey, saKeys[1].m_pCommand);
    }

    Button.HSplitTop(25.0f, 0, &Button);
    Button.HSplitTop(20.0f, &Button, 0);
    Label.HSplitTop(25.0f, 0, &Label);
    Label.HSplitTop(20.0f, &Label, 0);
    UI()->DoLabel(&Label, saKeys[2].m_Name, 18.0f*ms_FontmodHeight, -1);
    int MusicPrevKey = DoKeyReader(&sMusicPrevId, &Button, saKeys[2].m_KeyId);
    if(MusicPrevKey != saKeys[2].m_KeyId)
    {
        if(saKeys[2].m_KeyId != 0 || MusicPrevKey == 0)
            m_pClient->m_pBinds->Bind(saKeys[2].m_KeyId, "");
        if(MusicPrevKey != 0)
            m_pClient->m_pBinds->Bind(MusicPrevKey, saKeys[2].m_pCommand);
    }

    Button.HSplitTop(25.0f, 0, &Button);
    Button.HSplitTop(20.0f, &Button, 0);
    Label.HSplitTop(25.0f, 0, &Label);
    Label.HSplitTop(20.0f, &Label, 0);
    UI()->DoLabel(&Label, saKeys[3].m_Name, 18.0f*ms_FontmodHeight, -1);
    int MusicLouderKey = DoKeyReader(&sMusicLouderId, &Button, saKeys[3].m_KeyId);
    if(MusicLouderKey != saKeys[3].m_KeyId)
    {
        if(saKeys[3].m_KeyId != 0 || MusicLouderKey == 0)
            m_pClient->m_pBinds->Bind(saKeys[3].m_KeyId, "");
        if(MusicLouderKey != 0)
            m_pClient->m_pBinds->Bind(MusicLouderKey, saKeys[3].m_pCommand);
    }

    Button.HSplitTop(25.0f, 0, &Button);
    Button.HSplitTop(20.0f, &Button, 0);
    Label.HSplitTop(25.0f, 0, &Label);
    Label.HSplitTop(20.0f, &Label, 0);
    UI()->DoLabel(&Label, saKeys[4].m_Name, 18.0f*ms_FontmodHeight, -1);
    int MusicLowerKey = DoKeyReader(&sMusicLowerId, &Button, saKeys[4].m_KeyId);
    if(MusicLowerKey != saKeys[4].m_KeyId)
    {
        if(saKeys[4].m_KeyId != 0 || MusicLowerKey == 0)
            m_pClient->m_pBinds->Bind(saKeys[4].m_KeyId, "");
        if(MusicLowerKey != 0)
            m_pClient->m_pBinds->Bind(MusicLowerKey, saKeys[4].m_pCommand);
    }
    //ui
}
