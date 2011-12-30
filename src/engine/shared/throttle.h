/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_THROTTLE_H
#define ENGINE_SHARED_THROTTLE_H

#include <base/system.h>

class CThrottle
{
public:
	CThrottle();
	~CThrottle();

	bool Throttled(int Seconds);

private:
	int64 m_Time;
};

#endif // ENGINE_SHARED_THROTTLE_H
