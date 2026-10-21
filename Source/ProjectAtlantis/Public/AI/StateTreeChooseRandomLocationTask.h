#pragma once
#include "StateTreeTaskBase.h"
#include "StateTreeChooseRandomLocationTask.generated.h"

USTRUCT()
struct PROJECTATLANTIS_API FStateTreeChooseRandomLocationTaskInstanceData
{
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere, Category = Output)
	FVector GeneratedLocation = FVector::ZeroVector;
};

USTRUCT()
struct PROJECTATLANTIS_API FStateTreeChooseRandomLocationTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()
	
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FStateTreeChooseRandomLocationTaskInstanceData::StaticStruct();
	};
	
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	
	UPROPERTY(EditDefaultsOnly, Category = Parameter)
	FVector2D MaxRangeValues{1000.f, 1000.f};
};