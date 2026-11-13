// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AI/Patrol/AtlantisPatrolTypes.h"
#include "AtlantisPatrolComponent.generated.h"

class USplineComponent;

/**
 * Patrol state of an enemy: which route it follows and which stop it is heading to.
 * Holds no movement logic; State Tree tasks pick points here and move the enemy themselves.
 */
UCLASS(ClassGroup = (AI), meta = (BlueprintSpawnableComponent))
class PROJECTATLANTIS_API UAtlantisPatrolComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAtlantisPatrolComponent();

	/** True if PatrolRoute is set, implements the patrol route interface and has at least one point. */
	UFUNCTION(BlueprintPure, Category = "Atlantis|AI|Patrol")
	bool HasValidRoute() const;

	/** Moves the cursor to the closest or next route point. @return false if there is no valid route. */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|AI|Patrol")
	bool SelectPoint(EAtlantisPatrolPointSelection Selection);

	/** World location of the current route point. @return false if no point is selected. */
	UFUNCTION(BlueprintPure, Category = "Atlantis|AI|Patrol")
	bool GetCurrentPointLocation(FVector& OutLocation) const;

	/** Seconds to wait at the current route point. */
	UFUNCTION(BlueprintPure, Category = "Atlantis|AI|Patrol")
	float GetCurrentWaitTime() const;

protected:
	/** Route to follow (e.g. a BP_PatrolPath placed in the level). Its spline points must be in the water. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|AI|Patrol", meta = (AllowedClasses = "/Script/ProjectAtlantis.AtlantisPatrolRoute"))
	TObjectPtr<AActor> PatrolRoute;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Atlantis|AI|Patrol")
	FAtlantisPatrolCursor Cursor;

private:
	USplineComponent* GetRouteSpline() const;

	mutable bool bHasLoggedInvalidRoute = false;
};
