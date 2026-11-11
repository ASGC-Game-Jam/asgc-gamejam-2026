// Copyright (c) 2026 clarasmith-sound
// SPDX-License-Identifier: MIT

#pragma once

#include "Modules/ModuleManager.h"

/** The Sound Styles runtime module. Everything lives in its classes; the module itself has nothing to set up. */
class FSoundStylesModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
