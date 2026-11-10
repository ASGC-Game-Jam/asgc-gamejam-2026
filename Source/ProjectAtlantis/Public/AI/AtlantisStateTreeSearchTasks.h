// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "StateTreePropertyRef.h"
#include "StructUtils/InstancedStruct.h"
#include "AI/AtlantisSearchPointSource.h"
#include "AtlantisStateTreeSearchTasks.generated.h"

class AAIController;

// Tasks for the Searching branch of ST_Enemy. They talk to the controller's UAtlantisDetectionComponent.

USTRUCT()
struct PROJECTATLANTIS_API FAtlantisStateTreeBeginSearchTaskInstanceData
{
	GENERATED_BODY()

	/** The enemy's AI controller (bound automatically from the tree context). */
	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> AIController = nullptr;

	/** Where the search starts (bind to the detection evaluator's LastKnownLocation). */
	UPROPERTY(EditAnywhere, Category = Input)
	FVector Origin = FVector::ZeroVector;
};

/**
 * Starts a new search around Origin: forgets which points were checked. Runs until the state exits and never
 * completes the state by itself (put it on the Searching parent state).
 */
USTRUCT(meta = (DisplayName = "Begin Search", Category = "Atlantis|AI"))
struct PROJECTATLANTIS_API FAtlantisStateTreeBeginSearchTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAtlantisStateTreeBeginSearchTaskInstanceData;

	FAtlantisStateTreeBeginSearchTask();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

USTRUCT()
struct PROJECTATLANTIS_API FAtlantisStateTreeFindSearchPointTaskInstanceData
{
	GENERATED_BODY()

	/** The enemy's AI controller (bound automatically from the tree context). */
	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> AIController = nullptr;

	/** How many points to check before giving up (bind to a tree parameter, e.g. SearchSweepCount). */
	UPROPERTY(EditAnywhere, Category = Input, meta = (ClampMin = 0, UIMin = 0))
	int32 MaxSweeps = 3;

	/** Receives the point to check. Bind to a parameter of the Searching state. */
	UPROPERTY(EditAnywhere, Category = Out)
	TStateTreePropertyRef<FVector> SearchPoint;
};

/**
 * Picks the next place to check and writes it into SearchPoint. Succeeds with a point; fails once MaxSweeps points
 * were checked or there's nothing left to check (go give up).
 */
USTRUCT(meta = (DisplayName = "Find Search Point", Category = "Atlantis|AI"))
struct PROJECTATLANTIS_API FAtlantisStateTreeFindSearchPointTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAtlantisStateTreeFindSearchPointTaskInstanceData;

	FAtlantisStateTreeFindSearchPointTask();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

	/** How candidate points are chosen. */
	UPROPERTY(EditAnywhere, Category = Parameter, meta = (BaseStruct = "/Script/ProjectAtlantis.AtlantisSearchPointSource", ExcludeBaseStruct))
	FInstancedStruct Source;
};

USTRUCT()
struct PROJECTATLANTIS_API FAtlantisStateTreeLookAroundTaskInstanceData
{
	GENERATED_BODY()

	/** The enemy's AI controller (bound automatically from the tree context). */
	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> AIController = nullptr;

	/** Total time spent looking around. */
	UPROPERTY(EditAnywhere, Category = Parameter, meta = (ClampMin = 0.1, UIMin = 0.1, ForceUnits = "s"))
	float Duration = 4.f;

	/** How far left/right to glance, from the starting direction. */
	UPROPERTY(EditAnywhere, Category = Parameter, meta = (ClampMin = 0, ClampMax = 180, UIMin = 0, UIMax = 180, ForceUnits = "deg"))
	float HalfAngle = 75.f;

	/** Number of glances, alternating right/left; with 2+ the last one looks back ahead. */
	UPROPERTY(EditAnywhere, Category = Parameter, meta = (ClampMin = 1, UIMin = 1, UIMax = 8))
	int32 GlanceCount = 3;

	float Elapsed = 0.f;
	float StartYaw = 0.f;
	int32 AppliedGlance = INDEX_NONE;
};

/**
 * Glances left and right while the state is active (turning at the controller's yaw rate; does not stop movement).
 * Succeeds after Duration.
 */
USTRUCT(meta = (DisplayName = "Look Around", Category = "Atlantis|AI"))
struct PROJECTATLANTIS_API FAtlantisStateTreeLookAroundTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAtlantisStateTreeLookAroundTaskInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

USTRUCT()
struct PROJECTATLANTIS_API FAtlantisStateTreeGiveUpSearchTaskInstanceData
{
	GENERATED_BODY()

	/** The enemy's AI controller (bound automatically from the tree context). */
	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> AIController = nullptr;
};

/**
 * Ends the search: forgets every player the enemy can't currently see, so it goes back to Idle (and its patrol).
 * Succeeds on the next tick.
 */
USTRUCT(meta = (DisplayName = "Give Up Search", Category = "Atlantis|AI"))
struct PROJECTATLANTIS_API FAtlantisStateTreeGiveUpSearchTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAtlantisStateTreeGiveUpSearchTaskInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};
