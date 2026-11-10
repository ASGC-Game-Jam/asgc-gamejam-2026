// Copyright (c) 2026 ASGC

#include "AI/AtlantisStateTreeSelectPatrolPointTask.h"

#include "AI/AtlantisPatrolComponent.h"
#include "StateTreeExecutionContext.h"

FAtlantisStateTreeSelectPatrolPointTask::FAtlantisStateTreeSelectPatrolPointTask()
{
	bShouldCallTick = false;
}

EStateTreeRunStatus FAtlantisStateTreeSelectPatrolPointTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	UAtlantisPatrolComponent* Patrol = InstanceData.Actor != nullptr ? InstanceData.Actor->FindComponentByClass<UAtlantisPatrolComponent>() : nullptr;
	if (Patrol == nullptr || !Patrol->SelectPoint(InstanceData.Selection))
	{
		return EStateTreeRunStatus::Failed;
	}

	FVector Location;
	if (!Patrol->GetCurrentPointLocation(Location))
	{
		return EStateTreeRunStatus::Failed;
	}

	if (FVector* LocationPtr = InstanceData.PointLocation.GetMutablePtr(Context))
	{
		*LocationPtr = Location;
	}
	if (float* WaitTimePtr = InstanceData.WaitTime.GetMutablePtr(Context))
	{
		*WaitTimePtr = Patrol->GetCurrentWaitTime();
	}

	return EStateTreeRunStatus::Succeeded;
}
