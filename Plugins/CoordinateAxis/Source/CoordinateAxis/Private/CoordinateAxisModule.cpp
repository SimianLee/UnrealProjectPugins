// CoordinateAxisModule.cpp
// CoordinateAxis 插件模块入口

#include "Modules/ModuleManager.h"

class FCoordinateAxisModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
	}

	virtual void ShutdownModule() override
	{
	}
};

IMPLEMENT_MODULE(FCoordinateAxisModule, CoordinateAxis)
