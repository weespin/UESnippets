#pragma once

#include "Modules/ModuleManager.h"

class FSplineHelper : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};