// Copyright (c) 2026 ASGC

#include "AI/AtlantisStateTreeDetectionEvaluator.h"

#include "AI/AtlantisDetectionComponent.h"
#include "AIController.h"
#include "StateTreeExecutionContext.h"

void FAtlantisStateTreeDetectionEvaluator::TreeStart(FStateTreeExecutionContext& Context) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.Detection = UAtlantisDetectionComponent::FindFor(InstanceData.AIController);
	ensureMsgf(InstanceData.Detection != nullptr, TEXT("[%hs] %s has no AtlantisDetectionComponent; the enemy will stay Idle. Add one to its controller."),
		__FUNCTION__, *GetNameSafe(InstanceData.AIController));
	Refresh(InstanceData);
}

void FAtlantisStateTreeDetectionEvaluator::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	Refresh(Context.GetInstanceData(*this));
}

void FAtlantisStateTreeDetectionEvaluator::Refresh(FInstanceDataType& InstanceData) const
{
	const UAtlantisDetectionComponent* Detection = InstanceData.Detection.Get();
	if (Detection == nullptr)
	{
		InstanceData.Status = EMonsterStatus::Idle;
		InstanceData.SuspectActor = nullptr;
		InstanceData.Suspicion = 0.f;
		return;
	}

	InstanceData.Status = Detection->GetStatus();
	InstanceData.SuspectActor = Detection->GetSuspect();
	InstanceData.Suspicion = Detection->GetMeter();
	Detection->GetLastKnownLocation(InstanceData.LastKnownLocation);
}
