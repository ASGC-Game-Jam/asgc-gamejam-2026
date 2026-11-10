// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "StateTreePropertyRef.h"
#include "AI/AtlantisPatrolTypes.h"
#include "AtlantisStateTreeSelectPatrolPointTask.generated.h"

USTRUCT()
struct PROJECTATLANTIS_API FAtlantisStateTreeSelectPatrolPointTaskInstanceData
{
	GENERATED_BODY()

	/** The patrolling pawn (needs a UAtlantisPatrolComponent). */
	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AActor> Actor = nullptr;

	/** Closest point to resume a patrol, or the next point to continue it. */
	UPROPERTY(EditAnywhere, Category = Parameter)
	EAtlantisPatrolPointSelection Selection = EAtlantisPatrolPointSelection::NextPoint;

	/** Receives the selected point's location. Bind to a parameter of the parent Patrol state. */
	UPROPERTY(EditAnywhere, Category = Out)
	TStateTreePropertyRef<FVector> PointLocation;

	/** Receives the wait time at the selected point. Bind to a parameter of the parent Patrol state. */
	UPROPERTY(EditAnywhere, Category = Out)
	TStateTreePropertyRef<float> WaitTime;
};

/**
 * Picks the closest or next patrol point from the pawn's UAtlantisPatrolComponent and writes its location and
 * wait time into the bound state parameters, so sibling states (Swim To, Delay) can read them.
 * Succeeds immediately; fails if the pawn has no valid patrol route.
 */
USTRUCT(meta = (DisplayName = "Select Patrol Point", Category = "Atlantis|AI"))
struct PROJECTATLANTIS_API FAtlantisStateTreeSelectPatrolPointTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAtlantisStateTreeSelectPatrolPointTaskInstanceData;

	FAtlantisStateTreeSelectPatrolPointTask();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
