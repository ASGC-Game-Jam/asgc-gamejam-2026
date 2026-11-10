// Copyright (c) 2026 ASGC

#include "AI/AtlantisStateTreeSetFocusTask.h"

#include "AIController.h"
#include "StateTreeExecutionContext.h"

void FAtlantisStateTreeSetFocusTask::ApplyFocus(FInstanceDataType& InstanceData) const
{
	if (InstanceData.AIController == nullptr)
	{
		return;
	}

	if (InstanceData.FocusTarget != nullptr)
	{
		InstanceData.AIController->SetFocus(InstanceData.FocusTarget, EAIFocusPriority::Gameplay);
	}
	else
	{
		InstanceData.AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}
	InstanceData.AppliedTarget = InstanceData.FocusTarget;
}

EStateTreeRunStatus FAtlantisStateTreeSetFocusTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	ApplyFocus(Context.GetInstanceData(*this));
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FAtlantisStateTreeSetFocusTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (InstanceData.AppliedTarget.Get() != InstanceData.FocusTarget)
	{
		ApplyFocus(InstanceData);
	}
	return EStateTreeRunStatus::Running;
}

void FAtlantisStateTreeSetFocusTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (InstanceData.AIController != nullptr)
	{
		InstanceData.AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}
	InstanceData.AppliedTarget.Reset();
}
