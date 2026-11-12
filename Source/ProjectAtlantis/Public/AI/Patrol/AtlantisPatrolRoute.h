// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AtlantisPatrolRoute.generated.h"

class USplineComponent;

UINTERFACE(BlueprintType, Blueprintable)
class PROJECTATLANTIS_API UAtlantisPatrolRoute : public UInterface
{
	GENERATED_BODY()
};

/**
 * A patrol route an enemy can follow: spline points visited in order (closed spline = loop, open = ping-pong).
 * Implemented by BP_PatrolPath. Always call through IAtlantisPatrolRoute::Execute_* (works for Blueprint implementers).
 */
class PROJECTATLANTIS_API IAtlantisPatrolRoute
{
	GENERATED_BODY()

public:
	/** Spline whose points are the patrol stops (world space). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Atlantis|AI|Patrol")
	USplineComponent* GetRouteSpline() const;

	/** Seconds to wait at the given spline point. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Atlantis|AI|Patrol")
	float GetWaitTimeAtPoint(int32 PointIndex) const;
};
