#include "AI/STTask_ChooseRandomLocation.h"

#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FSTTask_ChooseRandomLocation::EnterState(FStateTreeExecutionContext& Context,
                                                             const FStateTreeTransitionResult& Transition) const
{
	FStateTreeTaskCommonBase::EnterState(Context, Transition);
	
	FSTTask_ChooseRandomLocationInstanceData& InstanceData = Context.GetInstanceData<FSTTask_ChooseRandomLocationInstanceData>(*this);
	
	InstanceData.GeneratedLocation = FVector{FMath::RandRange(MaxRangeValues.X * -1.f, MaxRangeValues.X), FMath::RandRange(MaxRangeValues.Y * -1.f, MaxRangeValues.Y), 0.f};
	return EStateTreeRunStatus::Succeeded;
}
