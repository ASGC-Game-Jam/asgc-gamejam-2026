// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "AITypes.h"
#include "StateTreeTaskBase.h"
#include "AtlantisStateTreeSwimToTask.generated.h"

class AAIController;
class UPathFollowingComponent;

USTRUCT()
struct PROJECTATLANTIS_API FAtlantisStateTreeSwimToTaskInstanceData
{
	GENERATED_BODY()

	/** The enemy's AI controller (bound automatically from the tree context). */
	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> AIController = nullptr;

	/** Where to swim to (the swimmer's capsule centre will end up here). */
	UPROPERTY(EditAnywhere, Category = Input)
	FVector TargetLocation = FVector::ZeroVector;

	/** The move succeeds once the swimmer is this close to the target, in 3D. */
	UPROPERTY(EditAnywhere, Category = Parameter, meta = (ClampMin = 0, UIMin = 0, UIMax = 500, ForceUnits = "cm"))
	float AcceptanceRadius = 75.f;

	/** Swim straight to the target when the line is clear, skipping the nav graph. */
	UPROPERTY(EditAnywhere, Category = Parameter)
	bool bAllowDirectLeg = true;

	/**
	 * Re-plan when the target has moved this far from where the current path leads (e.g. chasing a player).
	 * 0 = plan once on enter.
	 */
	UPROPERTY(EditAnywhere, Category = Parameter, meta = (ClampMin = 0, UIMin = 0, UIMax = 1000, ForceUnits = "cm"))
	float RepathDistance = 0.f;

	/**
	 * Keep following the target: never succeeds. Holds position on arrival and swims again when the target moves
	 * away (uses RepathDistance while moving). Failed paths are retried instead of failing the task. For chases.
	 */
	UPROPERTY(EditAnywhere, Category = Parameter)
	bool bFollowTarget = false;

	FAIRequestID RequestId;
	FDelegateHandle MoveFinishedHandle;
	TWeakObjectPtr<UPathFollowingComponent> PathFollowing;
	/** Target the current path leads to. */
	FVector PlannedTarget = FVector::ZeroVector;
	/** World time of the last path request (re-plans are rate limited). */
	float LastPlanTime = -1.f;
};

/**
 * Swims the controlled pawn to a 3D location using the swimming nav graph and the controller's path following.
 * Succeeds on arrival; fails if the pawn isn't swimming, no path exists, or the move is blocked/aborted.
 * With RepathDistance it re-plans as the target moves; with Follow Target it keeps chasing until the state exits.
 */
USTRUCT(meta = (DisplayName = "Swim To", Category = "Atlantis|AI"))
struct PROJECTATLANTIS_API FAtlantisStateTreeSwimToTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAtlantisStateTreeSwimToTaskInstanceData;

	FAtlantisStateTreeSwimToTask();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

private:
	/** Plans and requests a move to TargetLocation. Running while moving, Succeeded if already there, Failed if no path. */
	EStateTreeRunStatus StartMove(FStateTreeExecutionContext& Context, FInstanceDataType& InstanceData) const;
	/** Unbinds from the current move; aborts it too when bAbort (the pawn stops and hovers). */
	void StopMove(FInstanceDataType& InstanceData, bool bAbort) const;
	bool IsMoving(const FInstanceDataType& InstanceData) const;
};
