// Copyright (c) 2026 clarasmith-sound
// SPDX-License-Identifier: MIT

#include "SoundStyles.h"

#define LOCTEXT_NAMESPACE "FSoundStylesModule"

void FSoundStylesModule::StartupModule()
{
	// Nothing to set up: the native tags register themselves, and the subsystem is created with the game instance.
}

void FSoundStylesModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSoundStylesModule, SoundStyles)