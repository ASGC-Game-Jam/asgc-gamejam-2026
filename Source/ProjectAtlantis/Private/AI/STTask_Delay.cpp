#include "AI/STTask_Delay.h"

#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FSTTask_Delay::EnterState(FStateTreeExecutionContext& Context,
                                              const FStateTreeTransitionResult& Transition) const
{
	FSTTask_DelayInstanceData& InstanceData = Context.GetInstanceData<FSTTask_DelayInstanceData>(*this);
	InstanceData.TimeLeft = DelayTime;
	return FStateTreeTaskCommonBase::EnterState(Context, Transition);
}

EStateTreeRunStatus FSTTask_Delay::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	Super::Tick(Context, DeltaTime);
	FSTTask_DelayInstanceData& InstanceData = Context.GetInstanceData<FSTTask_DelayInstanceData>(*this);

	InstanceData.TimeLeft -= DeltaTime;
	
	if (InstanceData.TimeLeft < 0.f)
		return EStateTreeRunStatus::Succeeded;
	
	return EStateTreeRunStatus::Running;
}
