#pragma once
#include "StateTreeTaskBase.h"
#include "STTask_ChooseRandomLocation.generated.h"

USTRUCT()
struct PROJECTATLANTIS_API FSTTask_ChooseRandomLocationInstanceData
{
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere, Category = Output)
	FVector GeneratedLocation;
};

USTRUCT()
struct PROJECTATLANTIS_API FSTTask_ChooseRandomLocation : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()
	
	virtual const UStruct* GetInstanceDataType() const override {return FSTTask_ChooseRandomLocationInstanceData::StaticStruct();};
	
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	
	UPROPERTY(EditDefaultsOnly, Category = Parameter)
	FVector2D MaxRangeValues{1000.f, 1000.f};
};