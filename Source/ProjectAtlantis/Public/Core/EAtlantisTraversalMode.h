// Copyright (c) 2026 ASGC
#pragma once

#include "CoreMinimal.h"
#include "EAtlantisTraversalMode.generated.h"


UENUM(BlueprintType)
enum class EAtlantisTraversalMode : uint8
{
	/** Not traversing: before spawn, or in an engine mode with no traversal meaning. */
	None,
	
	/** Walking, jumping, or falling in a breathable environment. */
	Terrestrial,
	
	/** Moving in water */
	Swimming,
	
	/** Attached to an underwater walkable surface */
	SurfaceWalking //TODO: Check how water walking works
	
};
