// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AI/Patrol/AtlantisPatrolRoute.h"
#include "AtlantisPatrolPath.generated.h"

class USplineComponent;

/**
 * Native base for patrol path actors (BP_PatrolPath). Implements IAtlantisPatrolRoute so Blueprint children
 * only need a spline component: the first USplineComponent on the actor is the route, every point waits DefaultWaitTime.
 */
UCLASS(Abstract, Blueprintable)
class PROJECTATLANTIS_API AAtlantisPatrolPath : public AActor, public IAtlantisPatrolRoute
{
	GENERATED_BODY()

public:
	/** Seconds an enemy waits at each spline point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol", meta = (ClampMin = "0", Units = "s"))
	double DefaultWaitTime = 0.0;

	virtual USplineComponent* GetRouteSpline_Implementation() const override;
	virtual float GetWaitTimeAtPoint_Implementation(int32 PointIndex) const override;
};
