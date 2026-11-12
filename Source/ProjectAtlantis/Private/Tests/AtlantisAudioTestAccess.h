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

	/** The Music Per Map table, so tests can add their own maps. Save it before changing it and restore it after. */
	static TMap<TSoftObjectPtr<UWorld>, TSoftObjectPtr<UMetaSoundSource>>& MusicPerMap()
	{
		return GetMutableDefault<UAtlantisAudioDeveloperSettings>()->MusicPerMap;
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
	static void StartTracking(UAtlantisAudioSubsystem* Subsystem, UWorld* World) { Subsystem->HandlePostWorldInitialization(World, UWorld::InitializationValues()); }
#if WITH_EDITOR
	static void NotifyPropertyChanged(UAtlantisAudioSubsystem* Subsystem, UObject* Object)
	{
		FPropertyChangedEvent Event(nullptr);
		Subsystem->HandleObjectPropertyChanged(Object, Event);
	}
#endif
};