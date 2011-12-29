/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include "throttle.h"

CThrottle::CThrottle()
{
	m_Time = time_get();
}

CThrottle::~CThrottle()
{
}

bool CThrottle::Throttled(int Seconds)
{
	if(time_get() >= m_Time)
	{
		m_Time = time_get() + Seconds * time_freq();
		return true;
	}
	return false;
}

