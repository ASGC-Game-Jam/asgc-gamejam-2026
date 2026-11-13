// Copyright (c) 2026 ASGC

#include "AI/Patrol/AtlantisPatrolPath.h"

#include "Components/SplineComponent.h"

USplineComponent* AAtlantisPatrolPath::GetRouteSpline_Implementation() const
{
	return FindComponentByClass<USplineComponent>();
}

float AAtlantisPatrolPath::GetWaitTimeAtPoint_Implementation(int32 PointIndex) const
{
	return static_cast<float>(FMath::Max(0.0, DefaultWaitTime));
}
