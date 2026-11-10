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

	FAIRequestID RequestId;
	FDelegateHandle MoveFinishedHandle;
	TWeakObjectPtr<UPathFollowingComponent> PathFollowing;
};

/**
 * Swims the controlled pawn to a 3D location using the swimming nav graph and the controller's path following.
 * Succeeds on arrival; fails if the pawn isn't swimming, no path exists, or the move is blocked/aborted.
 */
USTRUCT(meta = (DisplayName = "Swim To", Category = "Atlantis|AI"))
struct PROJECTATLANTIS_API FAtlantisStateTreeSwimToTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAtlantisStateTreeSwimToTaskInstanceData;

	FAtlantisStateTreeSwimToTask();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
