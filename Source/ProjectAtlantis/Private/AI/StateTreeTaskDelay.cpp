#include "AI/StateTreeTaskDelay.h"

#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FStateTreeTaskDelay::EnterState(FStateTreeExecutionContext& Context,
                                              const FStateTreeTransitionResult& Transition) const
{
	FStateTreeTaskDelayInstanceData& InstanceData = Context.GetInstanceData<FStateTreeTaskDelayInstanceData>(*this);
	InstanceData.TimeLeft = DelayTime;
	return FStateTreeTaskCommonBase::EnterState(Context, Transition);
}

EStateTreeRunStatus FStateTreeTaskDelay::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	Super::Tick(Context, DeltaTime);
	FStateTreeTaskDelayInstanceData& InstanceData = Context.GetInstanceData<FStateTreeTaskDelayInstanceData>(*this);

	InstanceData.TimeLeft -= DeltaTime;
	
	if (InstanceData.TimeLeft < 0.f)
	{
		return EStateTreeRunStatus::Succeeded;
	}
	
	return EStateTreeRunStatus::Running;
}
