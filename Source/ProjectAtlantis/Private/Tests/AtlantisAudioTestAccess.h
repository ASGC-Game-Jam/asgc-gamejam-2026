// Copyright (c) 2026 ASGC

#pragma once

#include "Audio/AtlantisAudioSubsystem.h"
#include "Audio/AtlantisMusicAxisMapping.h"

#include "Audio/AtlantisAudioDeveloperSettings.h"

using FAtlantisAudioPayloadRegistry = TMap<FGameplayTag, TSoftObjectPtr<const UScriptStruct>>;

// Declared as a friend in AtlantisAudioDeveloperSettings.h so tests can reach the protected registry.
struct FAtlantisAudioValidationTestAccess
{
	static FAtlantisAudioPayloadRegistry& Registry()
	{
		return GetMutableDefault<UAtlantisAudioDeveloperSettings>()->PayloadRegistry;
	}
};

// Declared as a friend in AtlantisAudioSubsystem.h so tests can drive the math without playing a level.
struct FAtlantisAudioSubsystemTestAccess
{
	static void SetMapping(UAtlantisAudioSubsystem* Subsystem, UAtlantisMusicAxisMapping* Mapping) { Subsystem->AxisMapping = Mapping; }
	static void UpdateAxes(UAtlantisAudioSubsystem* Subsystem) { Subsystem->UpdateMusicAxes(); }
	static void StopTracking(UAtlantisAudioSubsystem* Subsystem) { Subsystem->StopTrackingWorld(); }
	static float PlayerState(const UAtlantisAudioSubsystem* Subsystem) { return Subsystem->CurrentPlayerState; }
	static float Tension(const UAtlantisAudioSubsystem* Subsystem) { return Subsystem->CurrentTension; }
};