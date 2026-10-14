#pragma once
#include "StateTreeTaskBase.h"

#include "STTask_Delay.generated.h"

USTRUCT()
struct PROJECTATLANTIS_API FSTTask_DelayInstanceData
{
	GENERATED_BODY()
	
	float TimeLeft;
};

USTRUCT()
struct PROJECTATLANTIS_API FSTTask_Delay : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()
	
	virtual const UStruct* GetInstanceDataType() const override {return FSTTask_DelayInstanceData::StaticStruct();}
	
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	
	UPROPERTY(EditDefaultsOnly, Category = Parameter)
	float DelayTime = 5.f;
};
