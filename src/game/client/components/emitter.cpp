/* (c) MAP94. See www.n-lvl.com/ndc/nclient/ for more information. */
#include "emitter.h"

CParticleEmitter::CParticleEmitter()
{
	OnReset();
}

void CParticleEmitter::OnReset()
{
    //Testing
    m_Particle.SetDefault();
	m_Particle.m_Spr = 7;
	m_Particle.m_Vel = vec2(0, -440);
	m_Particle.m_LifeSpan = 0.5f + frandom()*0.5f;
	m_Particle.m_Rot = pi * 10.0f * (frandom() - 0.5f);
	m_Particle.m_Rotspeed = pi * 2;
	m_Particle.m_StartSize = 36.0f;
	m_Particle.m_EndSize = 0;
	m_Particle.m_Friction = 0.8;
	m_Particle.m_Gravity.y = frandom()*-500.0f;
	m_Particle.m_Gravity.x = 0.0f;
	m_Particle.m_FlowAffected = 5;
	m_Particle.m_Pos = vec2(1010, 550);
	m_Particle.m_Color = vec4(1,1,1, 0.1);

	m_ParticlesPerSec = 360;
	m_LastParticle = time_get();
}


void CParticleEmitter::OnRender()
{
    float FPS = 1.0f / Client()->FrameTime();
    while ((float)((time_get() - m_LastParticle) / (double)time_freq()) > FPS / (float)m_ParticlesPerSec / 60.0f)
    {
        m_LastParticle += 1 / (float)m_ParticlesPerSec * (float)time_freq();
        m_pClient->m_pParticles->Add(CParticles::GROUP_GENERAL, &m_Particle);
    }
}

CFlowEmitter::CFlowEmitter()
{
	OnReset();
}

void CFlowEmitter::OnReset()
{
    //Testing
    for (int i = 0; i < MAX_FLOWSETTINGS; i++)
    {
        m_aFlowSettings[i].m_Activ = false;
        m_aFlowSettings[i].m_Pos = vec2(0, 0);
        m_aFlowSettings[i].m_Vel = vec2(0, 0);
        m_aFlowSettings[i].m_VelRandomInAdd = 0.0f;
        m_aFlowSettings[i].m_VelRandomMulti = 1.0f;
        m_aFlowSettings[i].m_VelRandomAdd = 0.0f;
        m_aFlowSettings[i].m_Size = 10.0f;

    }
}


void CFlowEmitter::OnRender()
{
    for (int i = 0; i < MAX_FLOWSETTINGS; i++)
    {
        if (m_aFlowSettings[i].m_Activ)
        {
            m_pClient->m_pFlow->Add(m_aFlowSettings[i].m_Pos, m_aFlowSettings[i].m_Vel * (m_aFlowSettings[i].m_VelRandomAdd + m_aFlowSettings[i].m_VelRandomMulti * (frandom() + m_aFlowSettings[i].m_VelRandomInAdd)), m_aFlowSettings[i].m_Size);
        }
    }
}
