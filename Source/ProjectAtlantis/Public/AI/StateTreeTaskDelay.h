#pragma once
#include "StateTreeTaskBase.h"

#include "StateTreeTaskDelay.generated.h"

USTRUCT()
struct PROJECTATLANTIS_API FStateTreeTaskDelayInstanceData
{
	GENERATED_BODY()
	
	float TimeLeft;
};

USTRUCT()
struct PROJECTATLANTIS_API FStateTreeTaskDelay : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()
	
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FStateTreeTaskDelayInstanceData::StaticStruct();
	}
	
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	
	UPROPERTY(EditDefaultsOnly, Category = Parameter)
	float DelayTime = 5.f;
};
