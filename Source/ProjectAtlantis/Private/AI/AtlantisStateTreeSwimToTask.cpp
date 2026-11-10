// Copyright (c) 2026 ASGC

#include "AI/AtlantisStateTreeSwimToTask.h"

#include "AI/AtlantisAITypes.h"
#include "AI/AtlantisNavGraphSubsystem.h"
#include "AIController.h"
#include "Algo/Transform.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "NavigationData.h"
#include "Navigation/PathFollowingComponent.h"
#include "StateTreeAsyncExecutionContext.h"
#include "StateTreeExecutionContext.h"

namespace
{
	/** Shortest time between two path requests of one task. */
	constexpr float MinReplanInterval = 0.25f;
	/** Follow mode: swim again once the target is this much further than AcceptanceRadius. */
	constexpr float FollowSlack = 50.f;
}

FAtlantisStateTreeSwimToTask::FAtlantisStateTreeSwimToTask()
{
	// Ticks only matter for re-planning / following; plain moves finish from the move-finished callback.
	bShouldCallTick = true;
}

bool FAtlantisStateTreeSwimToTask::IsMoving(const FInstanceDataType& InstanceData) const
{
	const UPathFollowingComponent* PathFollowing = InstanceData.PathFollowing.Get();
	return PathFollowing != nullptr && PathFollowing->GetStatus() != EPathFollowingStatus::Idle
		&& InstanceData.RequestId.IsEquivalent(PathFollowing->GetCurrentRequestId());
}

EStateTreeRunStatus FAtlantisStateTreeSwimToTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.RequestId = FAIRequestID::InvalidRequest;
	InstanceData.LastPlanTime = -1.f;

	AAIController* AIController = InstanceData.AIController;
	APawn* Pawn = AIController != nullptr ? AIController->GetPawn() : nullptr;
	if (Pawn == nullptr || AIController->GetPathFollowingComponent() == nullptr)
	{
		UE_LOG(LogAtlantisAI, Error, TEXT("[%hs] Missing pawn or path following component on %s."), __FUNCTION__, *GetNameSafe(AIController));
		return EStateTreeRunStatus::Failed;
	}

	// Only swimmers use the swim graph. Land enemies fail here and the tree falls back (stretch goal: land pathing).
	const UPawnMovementComponent* Movement = Pawn->GetMovementComponent();
	if (Movement == nullptr || !Movement->IsSwimming())
	{
		static bool bHasLoggedNotSwimming = false;
		if (!bHasLoggedNotSwimming)
		{
			bHasLoggedNotSwimming = true;
			UE_LOG(LogAtlantisAI, Warning, TEXT("[%hs] %s is not swimming; Swim To only supports swimming enemies for now."), __FUNCTION__, *Pawn->GetName());
		}
		return EStateTreeRunStatus::Failed;
	}

	const EStateTreeRunStatus Status = StartMove(Context, InstanceData);
	// Following never finishes by itself: arriving means holding position, a failed path is retried on tick.
	return InstanceData.bFollowTarget ? EStateTreeRunStatus::Running : Status;
}

EStateTreeRunStatus FAtlantisStateTreeSwimToTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (!InstanceData.bFollowTarget && InstanceData.RepathDistance <= 0.f)
	{
		return EStateTreeRunStatus::Running;
	}

	const APawn* Pawn = InstanceData.AIController != nullptr ? InstanceData.AIController->GetPawn() : nullptr;
	if (Pawn == nullptr)
	{
		return EStateTreeRunStatus::Failed;
	}
	if (Pawn->GetWorld()->GetTimeSeconds() - InstanceData.LastPlanTime < MinReplanInterval)
	{
		return EStateTreeRunStatus::Running;
	}

	const bool bMoving = IsMoving(InstanceData);
	const bool bTargetMoved = InstanceData.RepathDistance > 0.f
		&& FVector::Dist(InstanceData.TargetLocation, InstanceData.PlannedTarget) > InstanceData.RepathDistance;
	const bool bFellBehind = InstanceData.bFollowTarget && !bMoving
		&& FVector::Dist(Pawn->GetActorLocation(), InstanceData.TargetLocation) > InstanceData.AcceptanceRadius + FollowSlack;

	if ((bMoving && bTargetMoved) || bFellBehind)
	{
		// Unbind first: replacing a move reports the old one as finished. Don't abort yet (that would stall the swimmer).
		const TWeakObjectPtr<UPathFollowingComponent> OldPathFollowing = InstanceData.PathFollowing;
		const FAIRequestID OldRequestId = InstanceData.RequestId;
		StopMove(InstanceData, /*bAbort*/ false);
		const EStateTreeRunStatus Status = StartMove(Context, InstanceData);

		// No new move replaced the old one (already there, or no path): stop the old move instead of letting it run untracked.
		UPathFollowingComponent* OldFollowing = OldPathFollowing.Get();
		if (Status != EStateTreeRunStatus::Running && OldFollowing != nullptr && OldFollowing->GetStatus() != EPathFollowingStatus::Idle
			&& OldRequestId.IsEquivalent(OldFollowing->GetCurrentRequestId()))
		{
			OldFollowing->AbortMove(*OldFollowing, FPathFollowingResultFlags::OwnerFinished, OldRequestId, EPathFollowingVelocityMode::Keep);
		}
		if (!InstanceData.bFollowTarget)
		{
			return Status;
		}
	}
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FAtlantisStateTreeSwimToTask::StartMove(FStateTreeExecutionContext& Context, FInstanceDataType& InstanceData) const
{
	AAIController* AIController = InstanceData.AIController;
	APawn* Pawn = AIController->GetPawn();
	UPathFollowingComponent* PathFollowing = AIController->GetPathFollowingComponent();

	InstanceData.PlannedTarget = InstanceData.TargetLocation;
	InstanceData.LastPlanTime = Pawn->GetWorld()->GetTimeSeconds();

	const FVector PawnCentre = Pawn->GetActorLocation();
	if (FVector::Dist(PawnCentre, InstanceData.TargetLocation) <= InstanceData.AcceptanceRadius)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	UAtlantisNavGraphSubsystem* NavGraph = Pawn->GetWorld()->GetSubsystem<UAtlantisNavGraphSubsystem>();
	if (NavGraph == nullptr)
	{
		return EStateTreeRunStatus::Failed;
	}

	FAtlantisNavPath SwimPath;
	const EAtlantisNavPathResult PathResult = NavGraph->FindPath(PawnCentre, InstanceData.TargetLocation, FAtlantisNavQueryFilter(), InstanceData.bAllowDirectLeg, SwimPath);
	if (PathResult != EAtlantisNavPathResult::Direct && PathResult != EAtlantisNavPathResult::Graph)
	{
		UE_LOG(LogAtlantisAI, Verbose, TEXT("[%hs] %s: no swim path to %s (%s)."), __FUNCTION__, *Pawn->GetName(),
			*InstanceData.TargetLocation.ToCompactString(), *UEnum::GetValueAsString(PathResult));
		return EStateTreeRunStatus::Failed;
	}

	// Path following measures from the feet, so shift the centre-space path down. Point 0 is the current feet location.
	const FVector CentreToFeetOffset = PawnCentre - Pawn->GetNavAgentLocation();
	TArray<FNavPathPoint> EnginePoints;
	SwimPath.MakeEnginePathPoints(CentreToFeetOffset, EnginePoints);

	TArray<FVector> Locations;
	Algo::Transform(EnginePoints, Locations, [](const FNavPathPoint& Point) { return Point.Location; });
	const FNavPathSharedPtr EnginePath = MakeShared<FNavigationPath>(Locations, nullptr);
	EnginePath->GetPathPoints() = MoveTemp(EnginePoints);

	FAIMoveRequest MoveRequest(Locations.Last());
	MoveRequest.SetAcceptanceRadius(InstanceData.AcceptanceRadius);
	MoveRequest.SetReachTestIncludesAgentRadius(false);
	MoveRequest.SetAllowPartialPath(false);
	MoveRequest.SetProjectGoalLocation(false);

	InstanceData.RequestId = AIController->RequestMove(MoveRequest, EnginePath);
	if (!InstanceData.RequestId.IsValid())
	{
		return EStateTreeRunStatus::Failed;
	}
	if (PathFollowing->GetStatus() == EPathFollowingStatus::Idle)
	{
		// Finished during the request (already there).
		return EStateTreeRunStatus::Succeeded;
	}

	InstanceData.PathFollowing = PathFollowing;
	if (!InstanceData.bFollowTarget)
	{
		InstanceData.MoveFinishedHandle = PathFollowing->OnRequestFinished.AddLambda(
			[WeakContext = Context.MakeWeakExecutionContext(), RequestId = InstanceData.RequestId](FAIRequestID FinishedRequestId, const FPathFollowingResult& Result)
			{
				if (RequestId.IsEquivalent(FinishedRequestId))
				{
					WeakContext.FinishTask(Result.IsSuccess() ? EStateTreeFinishTaskType::Succeeded : EStateTreeFinishTaskType::Failed);
				}
			});
	}

	return EStateTreeRunStatus::Running;
}

void FAtlantisStateTreeSwimToTask::StopMove(FInstanceDataType& InstanceData, bool bAbort) const
{
	UPathFollowingComponent* PathFollowing = InstanceData.PathFollowing.Get();
	if (PathFollowing == nullptr)
	{
		return;
	}

	// Unbind first: AbortMove broadcasts OnRequestFinished synchronously.
	if (InstanceData.MoveFinishedHandle.IsValid())
	{
		PathFollowing->OnRequestFinished.Remove(InstanceData.MoveFinishedHandle);
		InstanceData.MoveFinishedHandle.Reset();
	}

	if (bAbort && IsMoving(InstanceData))
	{
		// Leaving the state (e.g. guard became Alerted): stop and hover.
		PathFollowing->AbortMove(*PathFollowing, FPathFollowingResultFlags::OwnerFinished, InstanceData.RequestId, EPathFollowingVelocityMode::Reset);
	}

	InstanceData.PathFollowing.Reset();
	InstanceData.RequestId = FAIRequestID::InvalidRequest;
}

void FAtlantisStateTreeSwimToTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	StopMove(Context.GetInstanceData(*this), /*bAbort*/ true);
}
