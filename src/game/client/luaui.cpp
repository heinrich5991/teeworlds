/* (c) MAP94. See www.n-lvl.com/ndc/nclient/ for more information. */
#include "lua.h"
#include "components/flow.h"
#include "components/particles.h"
#include <game/generated/client_data.h>
#include <engine/serverbrowser.h>
#include <engine/textrender.h>
#include <engine/graphics.h>
#include <engine/sound.h>
#include <engine/graphics.h>
#include <game/client/lineinput.h>
#include <game/client/components/menus.h>
#include <game/client/components/chat.h>

float ms_FontmodHeight = 0.8f;

CLuaUi::CLuaUi()
{
    mem_zero(this, sizeof(CLuaUi));
}

CLuaUi::~CLuaUi()
{

}


vec4 CLuaUi::ButtonColorMul(const void *pID)
{
    if (!m_pClient->UI())
        return vec4(1,1,1,1);
    if (!m_pClient->UI()->ActiveItem())
        return vec4(1,1,1,1);

	if(m_pClient->UI()->ActiveItem() == pID)
		return vec4(1,1,1,0.5f);
	else if(m_pClient->UI()->HotItem() == pID)
		return vec4(1,1,1,1.5f);
	return vec4(1,1,1,1);
}

void CLuaUi::Tick()
{
    if (!m_Used)
        return;
    if (m_Type == LUAUIBUTTON)
    {
        int state = DoButton_Menu(&m_Id, m_pText, m_Checked, &m_Rect, m_Corners, m_Color);
        if (state != 0)
        {
            if (!m_pLuaFile->FunctionExist(m_pCallback))
                return;
            m_pLuaFile->FunctionPrepare(m_pCallback);
            m_pLuaFile->PushInteger(state);
            m_pLuaFile->FunctionExec();
        }
    }
    else if(m_Type == LUAUIEDITBOX)
    {
        DoEditBox(&m_Id, &m_Rect, m_pText, sizeof(m_pText), m_FontSize, &m_Offset, m_Hidden, m_Corners, m_Color);
    }
    else if(m_Type == LUAUILABEL)
    {
        CUIRect *pRect = &m_Rect;
        CUIRect Temp;
        pRect->HMargin(pRect->h>=20.0f?2.0f:1.0f, &Temp);
        m_pClient->TextRender()->TextColor(m_Color.r, m_Color.g, m_Color.b, m_Color.a);
        if (m_Align == -1)
            m_pClient->UI()->DoLabelScaled(&Temp, m_pText, m_FontSize*ms_FontmodHeight, m_Align, Temp.w);
        else
            m_pClient->UI()->DoLabelScaled(&Temp, m_pText, m_FontSize*ms_FontmodHeight, m_Align);
        m_pClient->TextRender()->TextColor(1, 1, 1, 1);
    }
    else if(m_Type == LUAUIRECT)
    {
        CUIRect *pRect = &m_Rect;
        m_pClient->RenderTools()->DrawUIRect(pRect, m_Color, m_Corners, m_Rounding);
    }
    else if(m_Type == LUAUIIMAGE)
    {
        CUIRect *pRect = &m_Rect;
        int state = DoImage(&m_Id, m_TextureID, m_SpriteID, pRect);
        if (state != 0)
        {
            if (!m_pLuaFile->FunctionExist(m_pCallback))
                return;
            m_pLuaFile->FunctionPrepare(m_pCallback);
            m_pLuaFile->PushInteger(state);
            m_pLuaFile->FunctionExec();
        }
    }
    else if(m_Type == LUAUIIMAGEEX)
    {
        CUIRect *pRect = &m_Rect;
        int state = DoImageEx(&m_Id, m_TextureID, pRect, m_ClipX1,  m_ClipY1,  m_ClipX2,  m_ClipY2);
        if (state != 0)
        {
            if (!m_pLuaFile->FunctionExist(m_pCallback))
                return;
            m_pLuaFile->FunctionPrepare(m_pCallback);
            m_pLuaFile->PushInteger(state);
            m_pLuaFile->FunctionExec();
        }
    }
    else if(m_Type == LUAUILINE)
    {
        m_pClient->Graphics()->TextureSet(-1);
        m_pClient->Graphics()->BlendNormal();
        m_pClient->Graphics()->LinesBegin();
        m_pClient->Graphics()->SetColor(m_Color.r, m_Color.g, m_Color.b, m_Color.a);
        IGraphics::CLineItem Line;
        Line = IGraphics::CLineItem(m_Rect.x, m_Rect.y, m_Rect.w, m_Rect.h);
        m_pClient->Graphics()->LinesDraw(&Line, 1);
        m_pClient->Graphics()->LinesEnd();
    }
    else if(m_Type == LUAUISLIDER)
    {
        float NewValue = 0;
        if (m_Direction == 0)
        {
            NewValue = DoScrollbarH(&m_Id, &m_Rect, m_Value, m_Color);
        }
        else
        {
            NewValue = DoScrollbarV(&m_Id, &m_Rect, m_Value, m_Color);
        }
        if (m_Value != NewValue)
        {
            m_Value = NewValue;
            if (!m_pLuaFile->FunctionExist(m_pCallback))
                return;
            m_pLuaFile->FunctionPrepare(m_pCallback);
            m_pLuaFile->PushFloat(m_Value);
            m_pLuaFile->FunctionExec();
        }
    }
}

int CLuaUi::DoButton_Menu(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Corners, vec4 Color)
{
	m_pClient->RenderTools()->DrawUIRect(pRect, Color*ButtonColorMul(pID), Corners, 5.0f);
	CUIRect Temp;
	pRect->HMargin(pRect->h>=20.0f?2.0f:1.0f, &Temp);
	m_pClient->UI()->DoLabel(&Temp, pText, Temp.h*ms_FontmodHeight, 0);
	return m_pClient->UI()->DoButtonLogic(pID, pText, Checked, pRect);
}

int CLuaUi::DoEditBox(void *pID, const CUIRect *pRect, char *pStr, unsigned StrSize, float FontSize, float *Offset, bool Hidden, int Corners, vec4 Color)
{
    int Inside = m_pClient->UI()->MouseInside(pRect);
	bool ReturnValue = false;
	bool UpdateOffset = false;
	static int s_AtIndex = 0;
	static bool s_DoScroll = false;
	static float s_ScrollStart = 0.0f;

	FontSize *= m_pClient->UI()->Scale();

	if(m_pClient->UI()->LastActiveItem() == pID)
	{
		int Len = str_length(pStr);
		if(Len == 0)
			s_AtIndex = 0;

		if(Inside && m_pClient->UI()->MouseButton(0))
		{
			s_DoScroll = true;
			s_ScrollStart = m_pClient->UI()->MouseX();
			int MxRel = (int)(m_pClient->UI()->MouseX() - pRect->x);

			for(int i = 1; i <= Len; i++)
			{
				if(m_pClient->TextRender()->TextWidth(0, FontSize, pStr, i) - *Offset + 10 > MxRel)
				{
					s_AtIndex = i - 1;
					break;
				}

				if(i == Len)
					s_AtIndex = Len;
			}
		}
		else if(!m_pClient->UI()->MouseButton(0))
			s_DoScroll = false;
		else if(s_DoScroll)
		{
			// do scrolling
			if(m_pClient->UI()->MouseX() < pRect->x && s_ScrollStart-m_pClient->UI()->MouseX() > 10.0f)
			{
				s_AtIndex = max(0, s_AtIndex-1);
				s_ScrollStart = m_pClient->UI()->MouseX();
				UpdateOffset = true;
			}
			else if(m_pClient->UI()->MouseX() > pRect->x+pRect->w && m_pClient->UI()->MouseX()-s_ScrollStart > 10.0f)
			{
				s_AtIndex = min(Len, s_AtIndex+1);
				s_ScrollStart = m_pClient->UI()->MouseX();
				UpdateOffset = true;
			}
		}

		for(int i = 0; i < m_pClient->m_pMenus->GetNumInputEvents(); i++)
		{
			Len = str_length(pStr);
			ReturnValue |= CLineInput::Manipulate(m_pClient->m_pMenus->GetInputEvents()[i], pStr, StrSize, &Len, &s_AtIndex);
		}
	}

	bool JustGotActive = false;

	if(m_pClient->UI()->ActiveItem() == pID)
	{
		if(!m_pClient->UI()->MouseButton(0))
		{
			s_DoScroll = false;
			m_pClient->UI()->SetActiveItem(0);
		}
	}
	else if(m_pClient->UI()->HotItem() == pID)
	{
		if(m_pClient->UI()->MouseButton(0))
		{
			if (m_pClient->UI()->LastActiveItem() != pID)
				JustGotActive = true;
			m_pClient->UI()->SetActiveItem(pID);
		}
	}

	if(Inside)
		m_pClient->UI()->SetHotItem(pID);

	CUIRect Textbox = *pRect;
	m_pClient->RenderTools()->DrawUIRect(&Textbox, Color, Corners, 3.0f);
	Textbox.VMargin(2.0f, &Textbox);
	Textbox.HMargin(2.0f, &Textbox);

	const char *pDisplayStr = pStr;
	char aStars[128];

	if(Hidden)
	{
		unsigned s = str_length(pStr);
		if(s >= sizeof(aStars))
			s = sizeof(aStars)-1;
		for(unsigned int i = 0; i < s; ++i)
			aStars[i] = '*';
		aStars[s] = 0;
		pDisplayStr = aStars;
	}

	// check if the text has to be moved
	if(m_pClient->UI()->LastActiveItem() == pID && !JustGotActive && (UpdateOffset || m_pClient->m_pMenus->GetNumInputEvents()))
	{
		float w = m_pClient->TextRender()->TextWidth(0, FontSize, pDisplayStr, s_AtIndex);
		if(w-*Offset > Textbox.w)
		{
			// move to the left
			float wt = m_pClient->TextRender()->TextWidth(0, FontSize, pDisplayStr, -1);
			do
			{
				*Offset += min(wt-*Offset-Textbox.w, Textbox.w/3);
			}
			while(w-*Offset > Textbox.w);
		}
		else if(w-*Offset < 0.0f)
		{
			// move to the right
			do
			{
				*Offset = max(0.0f, *Offset-Textbox.w/3);
			}
			while(w-*Offset < 0.0f);
		}
	}
	m_pClient->UI()->ClipEnable(pRect);
	Textbox.x -= *Offset;

	m_pClient->UI()->DoLabel(&Textbox, pDisplayStr, FontSize, -1);

	// render the cursor
	if(m_pClient->UI()->LastActiveItem() == pID && !JustGotActive)
	{
		float w = m_pClient->TextRender()->TextWidth(0, FontSize, pDisplayStr, s_AtIndex);
		Textbox = *pRect;
		Textbox.VSplitLeft(2.0f, 0, &Textbox);
		Textbox.x += (w-*Offset-m_pClient->TextRender()->TextWidth(0, FontSize, "|", -1)/2);

		if((2*time_get()/time_freq()) % 2)	// make it blink
			m_pClient->UI()->DoLabel(&Textbox, "|", FontSize, -1);
	}
	m_pClient->UI()->ClipDisable();

	return ReturnValue;
}

int CLuaUi::DoImage(int *pID, int TextureID, int SpriteID, const CUIRect *pRect)
{
	m_pClient->Graphics()->TextureSet(TextureID);
	m_pClient->Graphics()->QuadsBegin();

    int Inside = m_pClient->UI()->MouseInside(pRect);
    int ret = 0;
    if(m_pClient->UI()->ActiveItem() == pID)
    {
        if(!m_pClient->UI()->MouseButton(0))
        {
            m_pClient->UI()->SetActiveItem(0);
            if (Inside)
                ret = 1;
        }
    }
    else if(m_pClient->UI()->HotItem() == pID)
    {
        if(m_pClient->UI()->MouseButton(0))
            m_pClient->UI()->SetActiveItem(pID);
    }
    m_pClient->RenderTools()->SelectSprite(SpriteID);

    if(Inside)
        m_pClient->UI()->SetHotItem(pID);



	IGraphics::CQuadItem QuadItem(pRect->x, pRect->y, pRect->w, pRect->h);
	m_pClient->Graphics()->QuadsDrawTL(&QuadItem, 1);
	m_pClient->Graphics()->QuadsEnd();

	return ret;
}

int CLuaUi::DoImageEx(int *pID, int TextureID, const CUIRect *pRect, float ClipX1, float ClipY1, float ClipX2, float ClipY2)
{
	m_pClient->Graphics()->TextureSet(TextureID);
	m_pClient->Graphics()->QuadsBegin();

    int Inside = m_pClient->UI()->MouseInside(pRect);
    int ret = 0;
    if(m_pClient->UI()->ActiveItem() == pID)
    {
        if(!m_pClient->UI()->MouseButton(0))
        {
            m_pClient->UI()->SetActiveItem(0);
            if (Inside)
                ret = 1;
        }
    }
    else if(m_pClient->UI()->HotItem() == pID)
    {
        if(m_pClient->UI()->MouseButton(0))
            m_pClient->UI()->SetActiveItem(pID);
    }

    if(Inside)
        m_pClient->UI()->SetHotItem(pID);


    m_pClient->Graphics()->QuadsSetSubset(ClipX1, ClipY1, ClipX2, ClipY2);
	IGraphics::CQuadItem QuadItem(pRect->x, pRect->y, pRect->w, pRect->h);
	m_pClient->Graphics()->QuadsDrawTL(&QuadItem, 1);
	m_pClient->Graphics()->QuadsEnd();

	return ret;
}

float CLuaUi::DoScrollbarV(const void *pID, const CUIRect *pRect, float Current, vec4 Color)
{
	CUIRect Handle;
	static float OffsetY;
	pRect->HSplitTop(33, &Handle, 0);

	Handle.y += (pRect->h-Handle.h)*Current;

	// logic
    float ReturnValue = Current;
    int Inside = m_pClient->UI()->MouseInside(&Handle);

	if(m_pClient->UI()->ActiveItem() == pID)
	{
		if(!m_pClient->UI()->MouseButton(0))
			m_pClient->UI()->SetActiveItem(0);

		float Min = pRect->y;
		float Max = pRect->h-Handle.h;
		float Cur = m_pClient->UI()->MouseY()-OffsetY;
		ReturnValue = (Cur-Min)/Max;
		if(ReturnValue < 0.0f) ReturnValue = 0.0f;
		if(ReturnValue > 1.0f) ReturnValue = 1.0f;
	}
	else if(m_pClient->UI()->HotItem() == pID)
	{
		if(m_pClient->UI()->MouseButton(0))
		{
			m_pClient->UI()->SetActiveItem(pID);
			OffsetY = m_pClient->UI()->MouseY()-Handle.y;
		}
	}

	if(Inside)
		m_pClient->UI()->SetHotItem(pID);

	// render
	CUIRect Rail;
	pRect->VMargin(5.0f, &Rail);
	m_pClient->RenderTools()->DrawUIRect(&Rail, Color, 0, 0.0f);

	CUIRect Slider = Handle;
	Slider.w = Rail.x-Slider.x;
	m_pClient->RenderTools()->DrawUIRect(&Slider, Color, CUI::CORNER_L, 2.5f);
	Slider.x = Rail.x+Rail.w;
	m_pClient->RenderTools()->DrawUIRect(&Slider, Color, CUI::CORNER_R, 2.5f);

	Slider = Handle;
	Slider.Margin(5.0f, &Slider);
	m_pClient->RenderTools()->DrawUIRect(&Slider, vec4(1,1,1,0.25f)*ButtonColorMul(pID), CUI::CORNER_ALL, 2.5f);

    return ReturnValue;
}

float CLuaUi::DoScrollbarH(const void *pID, const CUIRect *pRect, float Current, vec4 Color)
{
	CUIRect Handle;
	static float OffsetX;
	pRect->VSplitLeft(33, &Handle, 0);

	Handle.x += (pRect->w-Handle.w)*Current;

	// logic
    float ReturnValue = Current;
    int Inside = m_pClient->UI()->MouseInside(&Handle);

	if(m_pClient->UI()->ActiveItem() == pID)
	{
		if(!m_pClient->UI()->MouseButton(0))
			m_pClient->UI()->SetActiveItem(0);

		float Min = pRect->x;
		float Max = pRect->w-Handle.w;
		float Cur = m_pClient->UI()->MouseX()-OffsetX;
		ReturnValue = (Cur-Min)/Max;
		if(ReturnValue < 0.0f) ReturnValue = 0.0f;
		if(ReturnValue > 1.0f) ReturnValue = 1.0f;
	}
	else if(m_pClient->UI()->HotItem() == pID)
	{
		if(m_pClient->UI()->MouseButton(0))
		{
			m_pClient->UI()->SetActiveItem(pID);
			OffsetX = m_pClient->UI()->MouseX()-Handle.x;
		}
	}

	if(Inside)
		m_pClient->UI()->SetHotItem(pID);

	// render
	CUIRect Rail;
	pRect->HMargin(5.0f, &Rail);
	m_pClient->RenderTools()->DrawUIRect(&Rail, Color, 0, 0.0f);

	CUIRect Slider = Handle;
	Slider.h = Rail.y-Slider.y;
	m_pClient->RenderTools()->DrawUIRect(&Slider, Color, CUI::CORNER_T, 2.5f);
	Slider.y = Rail.y+Rail.h;
	m_pClient->RenderTools()->DrawUIRect(&Slider, Color, CUI::CORNER_B, 2.5f);

	Slider = Handle;
	Slider.Margin(5.0f, &Slider);
	m_pClient->RenderTools()->DrawUIRect(&Slider, vec4(1,1,1,0.25f)*ButtonColorMul(pID), CUI::CORNER_ALL, 2.5f);

    return ReturnValue;
}
