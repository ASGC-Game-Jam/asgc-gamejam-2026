// Copyright (c) 2026 clarasmith-sound
// SPDX-License-Identifier: MIT

#pragma once

#include "Modules/ModuleManager.h"

/**
 * The Sound Styles editor module. Registers the Sound Style Button's Details customization.
 * (The table's asset definition and factory are UCLASSes, so the engine finds them without registration.)
 */
class FSoundStylesEditorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
