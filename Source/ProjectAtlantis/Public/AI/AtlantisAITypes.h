// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "AtlantisAITypes.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAtlantisAI, Log, All);

UENUM()
enum class EMonsterStatus : uint8
{
	None = 0 UMETA(Hidden),
	// Not doing anything, has not perceived the player nor is searching for them.
	Idle = 1,
	// Actively pursuing a perceived target for which we have fresh stimuli for.
	Alerted = 2,
	// We've lost stimuli for this target, but we're still pursuing their last known location (or remaining idle at that location)
	Searching = 3,
	// Actively engaging the player in combat.
	Combat = 4
};