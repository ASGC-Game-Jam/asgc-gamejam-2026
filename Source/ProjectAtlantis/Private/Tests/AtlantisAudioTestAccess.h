// Copyright (c) 2026 ASGC

#pragma once

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
