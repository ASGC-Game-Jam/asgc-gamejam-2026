// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "AtlantisPatrolTypes.generated.h"

UENUM(BlueprintType)
enum class EAtlantisPatrolPointSelection : uint8
{
	/** The route point closest to the enemy (used when starting or resuming a patrol). */
	ClosestPoint,
	/** The next point along the route. */
	NextPoint
};

UENUM(BlueprintType)
enum class EAtlantisPatrolDirection : uint8
{
	Forward,
	Reverse
};

/** Where an enemy is along its patrol route. */
USTRUCT(BlueprintType)
struct PROJECTATLANTIS_API FAtlantisPatrolCursor
{
	GENERATED_BODY()

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Atlantis|AI|Patrol")
	int32 PointIndex = INDEX_NONE;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Atlantis|AI|Patrol")
	EAtlantisPatrolDirection Direction = EAtlantisPatrolDirection::Forward;
};

namespace Atlantis::AI
{
	/**
	 * Next stop along a route. Closed routes wrap around; open routes ping-pong between their ends.
	 * An unset cursor starts at point 0. Returns an unset cursor when the route has no points.
	 */
	PROJECTATLANTIS_API FAtlantisPatrolCursor AdvancePatrolCursor(const FAtlantisPatrolCursor& Cursor, int32 NumPoints, bool bClosedLoop);
}
