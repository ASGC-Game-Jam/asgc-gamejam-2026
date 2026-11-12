// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "AtlantisStateTreeSetFocusTask.generated.h"

class AAIController;

USTRUCT()
struct PROJECTATLANTIS_API FAtlantisStateTreeSetFocusTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> AIController = nullptr;

	/** Actor to keep facing. When empty, gameplay focus is cleared. */
	UPROPERTY(EditAnywhere, Category = Input)
	TObjectPtr<AActor> FocusTarget = nullptr;

	/** Target currently applied, so changes are re-applied. */
	TWeakObjectPtr<AActor> AppliedTarget;
};

/** Keeps the enemy facing an actor while the state is active. Runs until the state exits, then clears the focus. */
USTRUCT(meta = (DisplayName = "Set Focus", Category = "Atlantis|AI"))
struct PROJECTATLANTIS_API FAtlantisStateTreeSetFocusTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAtlantisStateTreeSetFocusTaskInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

private:
	void ApplyFocus(FInstanceDataType& InstanceData) const;
};
