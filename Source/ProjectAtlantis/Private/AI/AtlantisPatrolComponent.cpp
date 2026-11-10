// Copyright (c) 2026 ASGC

#include "AI/AtlantisPatrolComponent.h"

#include "AI/AtlantisAITypes.h"
#include "AI/AtlantisPatrolRoute.h"
#include "Components/SplineComponent.h"

UAtlantisPatrolComponent::UAtlantisPatrolComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

USplineComponent* UAtlantisPatrolComponent::GetRouteSpline() const
{
	if (PatrolRoute == nullptr || !PatrolRoute->Implements<UAtlantisPatrolRoute>())
	{
		if (PatrolRoute != nullptr && !bHasLoggedInvalidRoute)
		{
			bHasLoggedInvalidRoute = true;
			UE_LOG(LogAtlantisAI, Error, TEXT("[%hs] %s: patrol route %s doesn't implement AtlantisPatrolRoute."),
				__FUNCTION__, *GetNameSafe(GetOwner()), *PatrolRoute->GetName());
		}
		return nullptr;
	}
	return IAtlantisPatrolRoute::Execute_GetRouteSpline(PatrolRoute);
}

bool UAtlantisPatrolComponent::HasValidRoute() const
{
	const USplineComponent* Spline = GetRouteSpline();
	return Spline != nullptr && Spline->GetNumberOfSplinePoints() > 0;
}

bool UAtlantisPatrolComponent::SelectPoint(EAtlantisPatrolPointSelection Selection)
{
	const USplineComponent* Spline = GetRouteSpline();
	const int32 NumPoints = Spline != nullptr ? Spline->GetNumberOfSplinePoints() : 0;
	if (NumPoints <= 0)
	{
		return false;
	}

	if (Selection == EAtlantisPatrolPointSelection::NextPoint)
	{
		Cursor = Atlantis::AI::AdvancePatrolCursor(Cursor, NumPoints, Spline->IsClosedLoop());
		return true;
	}

	// Closest point in 3D (enemies swim, height matters).
	const FVector OwnerLocation = GetOwner()->GetActorLocation();
	int32 ClosestIndex = 0;
	float ClosestDistanceSq = TNumericLimits<float>::Max();
	for (int32 Index = 0; Index < NumPoints; ++Index)
	{
		const float DistanceSq = FVector::DistSquared(OwnerLocation, Spline->GetLocationAtSplinePoint(Index, ESplineCoordinateSpace::World));
		if (DistanceSq < ClosestDistanceSq)
		{
			ClosestDistanceSq = DistanceSq;
			ClosestIndex = Index;
		}
	}
	Cursor.PointIndex = ClosestIndex;
	return true;
}

bool UAtlantisPatrolComponent::GetCurrentPointLocation(FVector& OutLocation) const
{
	const USplineComponent* Spline = GetRouteSpline();
	if (Spline == nullptr || Cursor.PointIndex < 0 || Cursor.PointIndex >= Spline->GetNumberOfSplinePoints())
	{
		return false;
	}
	OutLocation = Spline->GetLocationAtSplinePoint(Cursor.PointIndex, ESplineCoordinateSpace::World);
	return true;
}

float UAtlantisPatrolComponent::GetCurrentWaitTime() const
{
	if (PatrolRoute == nullptr || Cursor.PointIndex == INDEX_NONE || !PatrolRoute->Implements<UAtlantisPatrolRoute>())
	{
		return 0.f;
	}
	return FMath::Max(0.f, IAtlantisPatrolRoute::Execute_GetWaitTimeAtPoint(PatrolRoute, Cursor.PointIndex));
}
