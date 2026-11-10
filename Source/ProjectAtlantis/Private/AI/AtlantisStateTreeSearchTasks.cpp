// Copyright (c) 2026 ASGC

#include "AI/AtlantisStateTreeSearchTasks.h"

#include "AI/AtlantisAITypes.h"
#include "AI/AtlantisDetectionComponent.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "StateTreeExecutionContext.h"

namespace
{
	UAtlantisDetectionComponent* FindDetection(const AAIController* AIController)
	{
		UAtlantisDetectionComponent* Detection = UAtlantisDetectionComponent::FindFor(AIController);
		static bool bHasLoggedMissingDetection = false;
		if (Detection == nullptr && !bHasLoggedMissingDetection)
		{
			bHasLoggedMissingDetection = true;
			UE_LOG(LogAtlantisAI, Warning, TEXT("[%hs] %s has no AtlantisDetectionComponent; search tasks do nothing. Add one to its controller."),
				__FUNCTION__, *GetNameSafe(AIController));
		}
		return Detection;
	}
}

// --- Begin Search ---

FAtlantisStateTreeBeginSearchTask::FAtlantisStateTreeBeginSearchTask()
{
	bShouldCallTick = false;
	// Lives on the Searching parent: its children decide when the search is over.
	bConsideredForCompletion = false;
}

EStateTreeRunStatus FAtlantisStateTreeBeginSearchTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (UAtlantisDetectionComponent* Detection = FindDetection(InstanceData.AIController))
	{
		Detection->BeginSearch(InstanceData.Origin);
	}
	return EStateTreeRunStatus::Running;
}

// --- Find Search Point ---

FAtlantisStateTreeFindSearchPointTask::FAtlantisStateTreeFindSearchPointTask()
{
	bShouldCallTick = false;
	Source.InitializeAs<FAtlantisSearchPointSource_NavGraph>();
}

EStateTreeRunStatus FAtlantisStateTreeFindSearchPointTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	UAtlantisDetectionComponent* Detection = FindDetection(InstanceData.AIController);
	const APawn* Pawn = InstanceData.AIController != nullptr ? InstanceData.AIController->GetPawn() : nullptr;
	const FAtlantisSearchPointSource* PointSource = Source.GetPtr<FAtlantisSearchPointSource>();
	if (Detection == nullptr || Pawn == nullptr || PointSource == nullptr)
	{
		return EStateTreeRunStatus::Failed;
	}

	const FAtlantisSearchMemory& Memory = Detection->GetSearchMemory();
	if (Memory.GetSweepsDone() >= InstanceData.MaxSweeps)
	{
		return EStateTreeRunStatus::Failed;
	}

	FAtlantisSearchPointQuery Query;
	Query.World = Pawn->GetWorld();
	Query.From = Pawn->GetActorLocation();
	Query.Origin = Memory.Origin;
	Query.Visited = Memory.Visited;

	FVector Point;
	if (!PointSource->FindPoint(Query, Point))
	{
		return EStateTreeRunStatus::Failed;
	}

	FVector* PointPtr = InstanceData.SearchPoint.GetMutablePtr(Context);
	if (PointPtr == nullptr)
	{
		static bool bHasLoggedUnboundSearchPoint = false;
		if (!bHasLoggedUnboundSearchPoint)
		{
			bHasLoggedUnboundSearchPoint = true;
			UE_LOG(LogAtlantisAI, Error, TEXT("[%hs] %s: Search Point is not bound; bind Search Point to a Searching state parameter."),
				__FUNCTION__, *GetNameSafe(InstanceData.AIController));
		}
		return EStateTreeRunStatus::Failed;
	}
	*PointPtr = Point;
	Detection->MarkSearchPointVisited(Point);
	return EStateTreeRunStatus::Succeeded;
}

// --- Look Around ---

EStateTreeRunStatus FAtlantisStateTreeLookAroundTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (InstanceData.AIController == nullptr || InstanceData.AIController->GetPawn() == nullptr)
	{
		return EStateTreeRunStatus::Failed;
	}
	InstanceData.Elapsed = 0.f;
	InstanceData.StartYaw = InstanceData.AIController->GetControlRotation().Yaw;
	InstanceData.AppliedGlance = INDEX_NONE;
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FAtlantisStateTreeLookAroundTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AAIController* AIController = InstanceData.AIController;
	const APawn* Pawn = AIController != nullptr ? AIController->GetPawn() : nullptr;
	if (Pawn == nullptr)
	{
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.Elapsed += DeltaTime;
	if (InstanceData.Elapsed >= InstanceData.Duration)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	const int32 NumGlances = FMath::Max(1, InstanceData.GlanceCount);
	const int32 Glance = FMath::Min(NumGlances - 1, FMath::FloorToInt32(InstanceData.Elapsed / InstanceData.Duration * NumGlances));
	if (Glance != InstanceData.AppliedGlance)
	{
		InstanceData.AppliedGlance = Glance;
		// Right, left, right... and the last glance looks back to where we started.
		const bool bLast = NumGlances > 1 && Glance == NumGlances - 1;
		const float Offset = bLast ? 0.f : InstanceData.HalfAngle * (Glance % 2 == 0 ? 1.f : -1.f);
		const FVector Direction = FRotator(0.f, InstanceData.StartYaw + Offset, 0.f).Vector();
		AIController->SetFocalPoint(Pawn->GetActorLocation() + Direction * 1000.f, EAIFocusPriority::Gameplay);
	}
	return EStateTreeRunStatus::Running;
}

void FAtlantisStateTreeLookAroundTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (InstanceData.AIController != nullptr)
	{
		InstanceData.AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}
}

// --- Give Up Search ---

EStateTreeRunStatus FAtlantisStateTreeGiveUpSearchTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (UAtlantisDetectionComponent* Detection = FindDetection(InstanceData.AIController))
	{
		Detection->ForgetLostTargets();
	}
	// Let the parent's Status transition pick this up next tick; succeed then as a safety net.
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FAtlantisStateTreeGiveUpSearchTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	return EStateTreeRunStatus::Succeeded;
}
