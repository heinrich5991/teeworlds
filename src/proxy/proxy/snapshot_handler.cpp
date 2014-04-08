#include "snapshot_handler.h"

	// for protocol_generated.h
	#include <engine/message.h>
#include "0.5/protocol_generated.h"
#include "0.6/protocol_generated.h"

#include <engine/shared/snapshot.h>

class CSnapshotHandler : public ISnapshotHandler
{
private:
	CSnapshotDelta m_SnapshotDelta;
public:
	CSnapshotHandler(int Version)
		: ISnapshotHandler()
	{
		switch(Version)
		{
		case VERSION_05:
			{
				Protocol5::CNetObjHandler NOH;
				for(int i = 0; i < Protocol5::NUM_NETOBJTYPES; i++)
					m_SnapshotDelta.SetStaticsize(i, NOH.GetObjSize(i));
				break;
			}
		case VERSION_06:
			{
				Protocol6::CNetObjHandler NOH;
				for(int i = 0; i < Protocol6::NUM_NETOBJTYPES; i++)
					m_SnapshotDelta.SetStaticsize(i, NOH.GetObjSize(i));
				break;
			}
		default:
			dbg_assert(false, "unreachable");
		}
	}

	virtual ~CSnapshotHandler()
	{
	}

	virtual int CreateDelta(CSnapshot *pFrom, CSnapshot *pTo, void *pData)
	{
		return m_SnapshotDelta.CreateDelta(pFrom, pTo, pData);
	}

	virtual int UnpackDelta(CSnapshot *pFrom, CSnapshot *pTo, void *pData, int DataSize)
	{
		if(pData == 0)
		{
			pData = m_SnapshotDelta.EmptyDelta();
			DataSize = sizeof(int) * 3;
		}
		return m_SnapshotDelta.UnpackDelta(pFrom, pTo, pData, DataSize);
	}

	virtual void *EmptyDelta()
	{
		return 0;
	}
};


ISnapshotHandler *CreateSnapshotHandler(int Version)
{
	dbg_assert(0 <= Version && Version < NUM_VERSIONS, "invalid snapshot_handler version");
	return new CSnapshotHandler(Version);
}
