// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Navigation/PathFollowingComponent.h"
#include "Core/EAtlantisTraversalMode.h"
#include "AtlantisPathFollowingComponent.generated.h"

UCLASS(ClassGroup = (AI))
class PROJECTATLANTIS_API UAtlantisPathFollowingComponent : public UPathFollowingComponent
{
	GENERATED_BODY()

public:
	UAtlantisPathFollowingComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Traversal mode of the segment currently being followed. */
	EAtlantisTraversalMode GetCurrentSegmentMode() const { return CurrentSegmentMode; }

protected:
	virtual void SetMoveSegment(int32 SegmentStartIndex) override;
	virtual void FollowPathSegment(float DeltaTime) override;
	virtual bool HasReachedDestination(const FVector& CurrentLocation) const override;
	virtual bool HasReachedCurrentTarget(const FVector& CurrentLocation) const override;
	virtual int32 DetermineStartingPathPoint(const FNavigationPath* ConsideredPath) const override;

	/** How close (in 3D) the agent must get to an intermediate swim-path point before moving on. */
	UPROPERTY(EditDefaultsOnly, Category = "Atlantis|AI|Navigation", meta = (ClampMin = 1, UIMin = 1, UIMax = 200, ForceUnits = "cm"))
	float IntermediatePointTolerance = 30.f;

	/**
	 * An intermediate point the agent has already moved past still counts as reached, but only within this distance.
	 * Further off the path (pushed by a collision, overshoot) the agent swims back to the point instead of cutting the corner.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Atlantis|AI|Navigation", meta = (ClampMin = 1, UIMin = 1, UIMax = 500, ForceUnits = "cm"))
	float PassedPointMaxDistance = 150.f;

private:
	/** True when following a swim-graph path (no navigation data) rather than a navmesh path. */
	bool IsFollowingGraphPath() const;

	EAtlantisTraversalMode CurrentSegmentMode = EAtlantisTraversalMode::None;
	bool bHasLoggedUnsupportedMode = false;
};
