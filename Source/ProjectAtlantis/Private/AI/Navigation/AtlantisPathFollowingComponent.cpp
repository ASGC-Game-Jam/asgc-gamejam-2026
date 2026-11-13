// Copyright (c) 2026 ASGC

#include "AI/Navigation/AtlantisPathFollowingComponent.h"

#include "AI/AtlantisAITypes.h"
#include "AI/Navigation/AtlantisNavGraph.h"
#include "NavigationData.h"

UAtlantisPathFollowingComponent::UAtlantisPathFollowingComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool UAtlantisPathFollowingComponent::IsFollowingGraphPath() const
{
	return Path.IsValid() && Path->GetNavigationDataUsed() == nullptr;
}

void UAtlantisPathFollowingComponent::SetMoveSegment(int32 SegmentStartIndex)
{
	Super::SetMoveSegment(SegmentStartIndex);

	CurrentSegmentMode = EAtlantisTraversalMode::None;
	if (IsFollowingGraphPath() && Path->GetPathPoints().IsValidIndex(SegmentStartIndex))
	{
		CurrentSegmentMode = Atlantis::AI::DecodeTraversalModeFlags(Path->GetPathPoints()[SegmentStartIndex].Flags);
	}
}

void UAtlantisPathFollowingComponent::FollowPathSegment(float DeltaTime)
{
	if (!IsFollowingGraphPath())
	{
		Super::FollowPathSegment(DeltaTime);
		return;
	}

	switch (CurrentSegmentMode)
	{
	case EAtlantisTraversalMode::Swimming:
		Super::FollowPathSegment(DeltaTime);
		break;

	default:
		// SurfaceWalking and others get their own handler here later.
		if (!bHasLoggedUnsupportedMode)
		{
			bHasLoggedUnsupportedMode = true;
			UE_LOG(LogAtlantisAI, Warning, TEXT("[%hs] %s: path segment uses unsupported traversal mode %s. Aborting move."),
				__FUNCTION__, *GetNameSafe(GetOwner()), *UEnum::GetValueAsString(CurrentSegmentMode));
		}
		OnPathFinished(FPathFollowingResult(EPathFollowingResult::Aborted, FPathFollowingResultFlags::InvalidPath));
		break;
	}
}

bool UAtlantisPathFollowingComponent::HasReachedDestination(const FVector& CurrentLocation) const
{
	if (!IsFollowingGraphPath())
	{
		return Super::HasReachedDestination(CurrentLocation);
	}

	const FVector GoalLocation = *Path->GetPathPointLocation(Path->GetPathPoints().Num() - 1);
	return FVector::DistSquared(CurrentLocation, GoalLocation) <= FMath::Square(FMath::Max(AcceptanceRadius, IntermediatePointTolerance));
}

bool UAtlantisPathFollowingComponent::HasReachedCurrentTarget(const FVector& CurrentLocation) const
{
	if (!IsFollowingGraphPath())
	{
		return Super::HasReachedCurrentTarget(CurrentLocation);
	}

	const FVector CurrentTarget = GetCurrentTargetLocation();
	const double DistanceSq = FVector::DistSquared(CurrentLocation, CurrentTarget);
	
	if (FVector::DotProduct(CurrentTarget - CurrentLocation, GetCurrentDirection()) < 0.
		&& DistanceSq <= FMath::Square(FMath::Max(PassedPointMaxDistance, IntermediatePointTolerance)))
	{
		return true;
	}

	return DistanceSq <= FMath::Square(IntermediatePointTolerance);
}

int32 UAtlantisPathFollowingComponent::DetermineStartingPathPoint(const FNavigationPath* ConsideredPath) const
{
	if (ConsideredPath != nullptr && ConsideredPath->IsValid() && ConsideredPath->GetNavigationDataUsed() == nullptr)
	{
		return 0;
	}
	return Super::DetermineStartingPathPoint(ConsideredPath);
}
