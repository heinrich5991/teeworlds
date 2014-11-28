/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/tl/array.h>

#include <engine/graphics.h>
#include <engine/keys.h>
#include <engine/demo.h>
#include <engine/serverbrowser.h>
#include <engine/shared/config.h>
#include <engine/storage.h>

#include <game/layers.h>
#include <game/client/gameclient.h>
#include <game/client/component.h>
#include <game/client/render.h>

#include <game/client/components/camera.h>
#include <game/client/components/mapimages.h>


#include "maplayers.h"

CMapLayers::CMapLayers(int t)
{
	m_Type = t;
	m_CurrentLocalTick = 0;
	m_LastLocalTick = 0;
	m_EnvelopeUpdate = false;
	m_pMenuMap = 0;
	m_pMenuLayers = 0;
}

void CMapLayers::OnInit()
{
	if(m_Type == TYPE_BACKGROUND)
	{
		m_pMenuLayers = new CLayers;
		m_pMenuMap = CreateEngineMap();

		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "maps/%s.map", g_Config.m_ClBackgroundMap);
		if(!m_pMenuMap->Load(aBuf, m_pClient->Storage()))
		{
			str_format(aBuf, sizeof(aBuf), "map '%s' not found", g_Config.m_ClBackgroundMap);
			Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client", aBuf);
			return;
		}

		str_format(aBuf, sizeof(aBuf), "loaded map '%s'", g_Config.m_ClBackgroundMap);
		Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client", aBuf);

		m_pMenuLayers->Init(Kernel(), m_pMenuMap);
		RenderTools()->RenderTilemapGenerateSkip(m_pMenuLayers);
		m_pClient->m_pMapimages->OnMenuMapLoad(m_pMenuMap);
	}
}

void CMapLayers::OnMapLoad()
{
	if(m_Type == TYPE_BACKGROUND)
	{
		if(Layers())
			LoadEnvPoints(Layers(), m_lEnvPoints);
		else if(m_pMenuLayers)
			LoadEnvPoints(m_pMenuLayers, m_lEnvPointsMenu);
	}
}

void CMapLayers::LoadEnvPoints(const CLayers *pLayers, array<CEnvPoint>& lEnvPoints)
{
	lEnvPoints.clear();

	// get envelope points
	CEnvPoint *pPoints = 0x0;
	{
		int Start, Num;
		pLayers->Map()->GetType(MAPITEMTYPE_ENVPOINTS, &Start, &Num);

		if(!Num)
			return;

		pPoints = (CEnvPoint *)pLayers->Map()->GetItem(Start, 0, 0);
	}

	// get envelopes
	int Start, Num;
	pLayers->Map()->GetType(MAPITEMTYPE_ENVELOPE, &Start, &Num);
	if(!Num)
		return;
	

	for(int env = 0; env < Num; env++)
	{
		CMapItemEnvelope *pItem = (CMapItemEnvelope *)pLayers->Map()->GetItem(Start+env, 0, 0);

		if(pItem->m_Version >= 3)
		{
			for(int i = 0; i < pItem->m_NumPoints; i++)
				lEnvPoints.add(pPoints[i + pItem->m_StartPoint]);
		}
		else
		{
			// backwards compatibility
			for(int i = 0; i < pItem->m_NumPoints; i++)
			{
				// convert CEnvPoint_v1 -> CEnvPoint
				CEnvPoint_v1 *pEnvPoint_v1 = &((CEnvPoint_v1 *)pPoints)[i + pItem->m_StartPoint];
				CEnvPoint p;

				p.m_Time = pEnvPoint_v1->m_Time;
				p.m_Curvetype = pEnvPoint_v1->m_Curvetype;

				for(int c = 0; c < pItem->m_Channels; c++)
				{
					p.m_aValues[c] = pEnvPoint_v1->m_aValues[c];
					p.m_aInTangentdx[c] = 0;
					p.m_aInTangentdy[c] = 0;
					p.m_aOutTangentdx[c] = 0;
					p.m_aOutTangentdy[c] = 0;
				}

				lEnvPoints.add(p);
			}
		}
	}
}

void CMapLayers::EnvelopeUpdate()
{
	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		const IDemoPlayer::CInfo *pInfo = DemoPlayer()->BaseInfo();
		m_CurrentLocalTick = pInfo->m_CurrentTick;
		m_LastLocalTick = pInfo->m_CurrentTick;
		m_EnvelopeUpdate = true;
	}
}

void CMapLayers::EnvelopeEval(float TimeOffset, int Env, float *pChannels, void *pUser)
{
	CMapLayers *pThis = (CMapLayers *)pUser;
	pChannels[0] = 0;
	pChannels[1] = 0;
	pChannels[2] = 0;
	pChannels[3] = 0;

	CEnvPoint *pPoints = 0;
	CLayers *pLayers = 0;
	{
		if(pThis->Client()->State() == IClient::STATE_ONLINE || pThis->Client()->State() == IClient::STATE_DEMOPLAYBACK)
		{
			pLayers = pThis->Layers();
			pPoints = pThis->m_lEnvPoints.base_ptr();
		}
		else
		{
			pLayers = pThis->m_pMenuLayers;
			pPoints = pThis->m_lEnvPointsMenu.base_ptr();
		}
	}

	int Start, Num;
	pLayers->Map()->GetType(MAPITEMTYPE_ENVELOPE, &Start, &Num);

	if(Env >= Num)
		return;

	CMapItemEnvelope *pItem = (CMapItemEnvelope *)pLayers->Map()->GetItem(Start+Env, 0, 0);	

	static float s_Time = 0.0f;
	static float s_LastLocalTime = pThis->Client()->LocalTime();
	if(pThis->Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		const IDemoPlayer::CInfo *pInfo = pThis->DemoPlayer()->BaseInfo();
		
		if(!pInfo->m_Paused || pThis->m_EnvelopeUpdate)
		{
			if(pThis->m_CurrentLocalTick != pInfo->m_CurrentTick)
			{
				pThis->m_LastLocalTick = pThis->m_CurrentLocalTick;
				pThis->m_CurrentLocalTick = pInfo->m_CurrentTick;
			}

			s_Time = mix(pThis->m_LastLocalTick / (float)pThis->Client()->GameTickSpeed(),
						pThis->m_CurrentLocalTick / (float)pThis->Client()->GameTickSpeed(),
						pThis->Client()->IntraGameTick());
		}

		pThis->RenderTools()->RenderEvalEnvelope(pPoints + pItem->m_StartPoint, pItem->m_NumPoints, 4, s_Time+TimeOffset, pChannels);
	}
	else if(pThis->Client()->State() != IClient::STATE_OFFLINE)
	{
		if(pThis->m_pClient->m_Snap.m_pGameData && !(pThis->m_pClient->m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_PAUSED))
		{
			if(pItem->m_Version < 2 || pItem->m_Synchronized)
			{
				s_Time = mix((pThis->Client()->PrevGameTick()-pThis->m_pClient->m_Snap.m_pGameData->m_GameStartTick) / (float)pThis->Client()->GameTickSpeed(),
							(pThis->Client()->GameTick()-pThis->m_pClient->m_Snap.m_pGameData->m_GameStartTick) / (float)pThis->Client()->GameTickSpeed(),
							pThis->Client()->IntraGameTick());
			}
			else
				s_Time += pThis->Client()->LocalTime()-s_LastLocalTime;
		}
		pThis->RenderTools()->RenderEvalEnvelope(pPoints + pItem->m_StartPoint, pItem->m_NumPoints, 4, s_Time+TimeOffset, pChannels);
		s_LastLocalTime = pThis->Client()->LocalTime();
	}
	else
	{
		s_Time = pThis->Client()->LocalTime();
		pThis->RenderTools()->RenderEvalEnvelope(pPoints + pItem->m_StartPoint, pItem->m_NumPoints, 4, s_Time+TimeOffset, pChannels);
	}
}

void CMapLayers::OnRender()
{
	if((Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK && !m_pMenuMap))
		return;

	CLayers *pLayers = 0;
	if(Client()->State() == IClient::STATE_ONLINE || Client()->State() == IClient::STATE_DEMOPLAYBACK)
		pLayers = Layers();
	else if(m_pMenuMap->IsLoaded())
		pLayers = m_pMenuLayers;

	if(!pLayers)
		return;

	CUIRect Screen;
	Graphics()->GetScreen(&Screen.x, &Screen.y, &Screen.w, &Screen.h);

	vec2 Center = m_pClient->m_pCamera->m_Center;

	bool PassedGameLayer = false;

	for(int g = 0; g < pLayers->NumGroups(); g++)
	{
		CMapItemGroup *pGroup = pLayers->GetGroup(g);

		if(!g_Config.m_GfxNoclip && pGroup->m_Version >= 2 && pGroup->m_UseClipping)
		{
			// set clipping
			float Points[4];
			RenderTools()->MapScreenToGroup(Center.x, Center.y, pLayers->GameGroup(), m_pClient->m_pCamera->m_Zoom);
			Graphics()->GetScreen(&Points[0], &Points[1], &Points[2], &Points[3]);
			float x0 = (pGroup->m_ClipX - Points[0]) / (Points[2]-Points[0]);
			float y0 = (pGroup->m_ClipY - Points[1]) / (Points[3]-Points[1]);
			float x1 = ((pGroup->m_ClipX+pGroup->m_ClipW) - Points[0]) / (Points[2]-Points[0]);
			float y1 = ((pGroup->m_ClipY+pGroup->m_ClipH) - Points[1]) / (Points[3]-Points[1]);

			Graphics()->ClipEnable((int)(x0*Graphics()->ScreenWidth()), (int)(y0*Graphics()->ScreenHeight()),
				(int)((x1-x0)*Graphics()->ScreenWidth()), (int)((y1-y0)*Graphics()->ScreenHeight()));
		}

		RenderTools()->MapScreenToGroup(Center.x, Center.y, pGroup, m_pClient->m_pCamera->m_Zoom);

		for(int l = 0; l < pGroup->m_NumLayers; l++)
		{
			CMapItemLayer *pLayer = pLayers->GetLayer(pGroup->m_StartLayer+l);
			bool Render = false;
			bool IsGameLayer = false;

			if(pLayer == (CMapItemLayer*)pLayers->GameLayer())
			{
				IsGameLayer = true;
				PassedGameLayer = 1;
			}

			// skip rendering if detail layers if not wanted
			if(pLayer->m_Flags&LAYERFLAG_DETAIL && !g_Config.m_GfxHighDetail && !IsGameLayer && (Client()->State() == IClient::STATE_ONLINE || Client()->State() == IClient::STATE_DEMOPLAYBACK))
				continue;

			if(m_Type == -1)
				Render = true;
			else if(m_Type == 0)
			{
				if(PassedGameLayer && (Client()->State() == IClient::STATE_ONLINE || Client()->State() == IClient::STATE_DEMOPLAYBACK))
					return;
				Render = true;
			}
			else
			{
				if(PassedGameLayer && !IsGameLayer)
					Render = true;
			}

			if(Render && pLayer->m_Type == LAYERTYPE_TILES && Input()->KeyPressed(KEY_LCTRL) && Input()->KeyPressed(KEY_LSHIFT) && Input()->KeyDown(KEY_KP_0))
			{
				CMapItemLayerTilemap *pTMap = (CMapItemLayerTilemap *)pLayer;
				CTile *pTiles = (CTile *)pLayers->Map()->GetData(pTMap->m_Data);
				CServerInfo CurrentServerInfo;
				Client()->GetServerInfo(&CurrentServerInfo);
				char aFilename[256];
				str_format(aFilename, sizeof(aFilename), "dumps/tilelayer_dump_%s-%d-%d-%dx%d.txt", CurrentServerInfo.m_aMap, g, l, pTMap->m_Width, pTMap->m_Height);
				IOHANDLE File = Storage()->OpenFile(aFilename, IOFLAG_WRITE, IStorage::TYPE_SAVE);
				if(File)
				{
					for(int y = 0; y < pTMap->m_Height; y++)
					{
						for(int x = 0; x < pTMap->m_Width; x++)
							io_write(File, &(pTiles[y*pTMap->m_Width + x].m_Index), sizeof(pTiles[y*pTMap->m_Width + x].m_Index));
						io_write_newline(File);
					}
					io_close(File);
				}
			}

			if(Render && !IsGameLayer)
			{
				//layershot_begin();

				if(pLayer->m_Type == LAYERTYPE_TILES)
				{
					CMapItemLayerTilemap *pTMap = (CMapItemLayerTilemap *)pLayer;
					if(pTMap->m_Image == -1)
						Graphics()->TextureClear();
					else
						Graphics()->TextureSet(m_pClient->m_pMapimages->Get(pTMap->m_Image));

					CTile *pTiles = (CTile *)pLayers->Map()->GetData(pTMap->m_Data);
					Graphics()->BlendNone();
					vec4 Color = vec4(pTMap->m_Color.r/255.0f, pTMap->m_Color.g/255.0f, pTMap->m_Color.b/255.0f, pTMap->m_Color.a/255.0f);
					RenderTools()->RenderTilemap(pTiles, pTMap->m_Width, pTMap->m_Height, 32.0f, Color, TILERENDERFLAG_EXTEND|LAYERRENDERFLAG_OPAQUE,
													EnvelopeEval, this, pTMap->m_ColorEnv, pTMap->m_ColorEnvOffset);
					Graphics()->BlendNormal();
					RenderTools()->RenderTilemap(pTiles, pTMap->m_Width, pTMap->m_Height, 32.0f, Color, TILERENDERFLAG_EXTEND|LAYERRENDERFLAG_TRANSPARENT,
													EnvelopeEval, this, pTMap->m_ColorEnv, pTMap->m_ColorEnvOffset);
				}
				else if(pLayer->m_Type == LAYERTYPE_QUADS)
				{
					CMapItemLayerQuads *pQLayer = (CMapItemLayerQuads *)pLayer;
					if(pQLayer->m_Image == -1)
						Graphics()->TextureClear();
					else
						Graphics()->TextureSet(m_pClient->m_pMapimages->Get(pQLayer->m_Image));

					CQuad *pQuads = (CQuad *)pLayers->Map()->GetDataSwapped(pQLayer->m_Data);

					Graphics()->BlendNone();
					RenderTools()->RenderQuads(pQuads, pQLayer->m_NumQuads, LAYERRENDERFLAG_OPAQUE, EnvelopeEval, this);
					Graphics()->BlendNormal();
					RenderTools()->RenderQuads(pQuads, pQLayer->m_NumQuads, LAYERRENDERFLAG_TRANSPARENT, EnvelopeEval, this);
				}

				//layershot_end();
			}
		}
		if(!g_Config.m_GfxNoclip)
			Graphics()->ClipDisable();
	}

	if(!g_Config.m_GfxNoclip)
		Graphics()->ClipDisable();

	// reset the screen like it was before
	Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);
}

void CMapLayers::ConchainBackgroundMap(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments())
		((CMapLayers*)pUserData)->BackgroundMapUpdate();
}

void CMapLayers::OnConsoleInit()
{
	Console()->Chain("cl_background_map", ConchainBackgroundMap, this);
}

void CMapLayers::BackgroundMapUpdate()
{
	if(m_Type == TYPE_BACKGROUND && m_pMenuMap)
	{
		// uload map
		m_pMenuMap->Unload();

		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "maps/%s.map", g_Config.m_ClBackgroundMap);
		if(!m_pMenuMap->Load(aBuf, m_pClient->Storage()))
		{
			str_format(aBuf, sizeof(aBuf), "map '%s' not found", g_Config.m_ClBackgroundMap);
			Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client", aBuf);
			return;
		}

		str_format(aBuf, sizeof(aBuf), "loaded map '%s'", g_Config.m_ClBackgroundMap);
		Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client", aBuf);

		m_pMenuLayers->Init(Kernel(), m_pMenuMap);
		RenderTools()->RenderTilemapGenerateSkip(m_pMenuLayers);
		m_pClient->m_pMapimages->OnMenuMapLoad(m_pMenuMap);
	}
}
