#include "AI/StateTreeChooseRandomLocationTask.h"

#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FStateTreeChooseRandomLocationTask::EnterState(FStateTreeExecutionContext& Context,
                                                             const FStateTreeTransitionResult& Transition) const
{
	FStateTreeTaskCommonBase::EnterState(Context, Transition);
	
	FStateTreeChooseRandomLocationTaskInstanceData& InstanceData = Context.GetInstanceData<FStateTreeChooseRandomLocationTaskInstanceData>(*this);
	
	InstanceData.GeneratedLocation = FVector{FMath::RandRange(MaxRangeValues.X * -1.f, MaxRangeValues.X), FMath::RandRange(MaxRangeValues.Y * -1.f, MaxRangeValues.Y), 0.f};
	return EStateTreeRunStatus::Succeeded;
}
