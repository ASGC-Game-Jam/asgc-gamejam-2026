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

FAtlantisStateTreeSwimToTask::FAtlantisStateTreeSwimToTask()
{
	// Everything happens on enter and in the move-finished callback.
	bShouldCallTick = false;
}

EStateTreeRunStatus FAtlantisStateTreeSwimToTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.RequestId = FAIRequestID::InvalidRequest;

	AAIController* AIController = InstanceData.AIController;
	APawn* Pawn = AIController != nullptr ? AIController->GetPawn() : nullptr;
	UPathFollowingComponent* PathFollowing = AIController != nullptr ? AIController->GetPathFollowingComponent() : nullptr;
	if (Pawn == nullptr || PathFollowing == nullptr)
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
	InstanceData.MoveFinishedHandle = PathFollowing->OnRequestFinished.AddLambda(
		[WeakContext = Context.MakeWeakExecutionContext(), RequestId = InstanceData.RequestId](FAIRequestID FinishedRequestId, const FPathFollowingResult& Result)
		{
			if (RequestId.IsEquivalent(FinishedRequestId))
			{
				WeakContext.FinishTask(Result.IsSuccess() ? EStateTreeFinishTaskType::Succeeded : EStateTreeFinishTaskType::Failed);
			}
		});

	return EStateTreeRunStatus::Running;
}

void FAtlantisStateTreeSwimToTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	UPathFollowingComponent* PathFollowing = InstanceData.PathFollowing.Get();
	if (PathFollowing == nullptr)
	{
		return;
	}

	// Unbind first: AbortMove broadcasts OnRequestFinished synchronously.
	PathFollowing->OnRequestFinished.Remove(InstanceData.MoveFinishedHandle);
	InstanceData.MoveFinishedHandle.Reset();

	if (PathFollowing->GetStatus() != EPathFollowingStatus::Idle && InstanceData.RequestId.IsEquivalent(PathFollowing->GetCurrentRequestId()))
	{
		// Leaving the state (e.g. guard became Alerted): stop and hover.
		PathFollowing->AbortMove(*PathFollowing, FPathFollowingResultFlags::OwnerFinished, InstanceData.RequestId, EPathFollowingVelocityMode::Reset);
	}

	InstanceData.PathFollowing.Reset();
	InstanceData.RequestId = FAIRequestID::InvalidRequest;
}
