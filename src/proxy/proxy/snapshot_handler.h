#ifndef PROXY_SNAPSHOT_HANDLER_H
#define PROXY_SNAPSHOT_HANDLER_H

#include "versions.h"

class CSnapshot;

class ISnapshotHandler
{
public:
	ISnapshotHandler() {}
	virtual ~ISnapshotHandler() {}

	virtual int CreateDelta(CSnapshot *pFrom, CSnapshot *pTo, void *pData) = 0;
	virtual int UnpackDelta(CSnapshot *pFrom, CSnapshot *pTo, void *pData, int DataSize) = 0;
	virtual void *EmptyDelta() = 0;
};

ISnapshotHandler *CreateSnapshotHandler(int Version);

#endif // PROXY_SNAPSHOT_HANDLER_H
