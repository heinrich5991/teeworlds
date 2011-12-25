/* (c) MAP94. See www.n-lvl.com/ndc/nclient/ for more information. */
#ifndef ENGINE_LUA_H
#define ENGINE_LUA_H

#include <engine/shared/protocol.h>

#include "kernel.h"

#define MAX_LUA_FILES 64

class ILua : public IInterface
{
	MACRO_INTERFACE("lua", 0)
public:
	virtual void Init() = 0;
	virtual char *GetFileDir(int i) = 0;
	virtual void AddLuaFile(char *pFilename, bool NoSave = false) = 0;
	virtual void DeleteLuaFile(int i) = 0;
	virtual bool GetLuaSaveOption(int i) = 0;
};

#endif
