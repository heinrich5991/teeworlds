/* (c) MAP94. See www.n-lvl.com/ndc/nclient/ for more information. */
#ifndef GAME_CLIENT_COMPONENTS_EMITTER_H
#define GAME_CLIENT_COMPONENTS_EMITTER_H
#include "particles.h"
#include "flow.h"
#include <game/client/component.h>

class CEmitter : public CComponent
{
    friend class CGameClient;
};

class CEmitters : public CComponent
{
    friend class CGameClient;

    enum
	{
		MAX_EMITTERS=256,
	};
	CEmitter m_aEmitters[MAX_EMITTERS];
};

class CParticleEmitter : public CEmitter
{
    friend class CGameClient;
public:
    CParticleEmitter();
    //Particle Settings
    CParticle m_Particle;

	//Emitter Settings
	int m_ParticlesPerSec;
    int64 m_LastParticle;

	void OnReset();
	void OnRender();
};

class CFlowEmitter : public CEmitter
{
    friend class CGameClient;

    enum
	{
		MAX_FLOWSETTINGS=32,
	};

public:
    CFlowEmitter();
    struct CFlowSettings
    {
        vec2 m_Pos;
        vec2 m_Vel;
        float m_VelRandomInAdd;
        float m_VelRandomMulti;
        float m_VelRandomAdd;
        float m_Size;
        bool m_Activ;
    };

    CFlowSettings m_aFlowSettings[MAX_FLOWSETTINGS];

	void OnReset();
	void OnRender();
};

#endif
