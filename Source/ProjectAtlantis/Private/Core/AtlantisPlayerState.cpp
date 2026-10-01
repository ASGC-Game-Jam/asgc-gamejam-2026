// Copyright (c) 2026 ASGC


#include "Core/AtlantisPlayerState.h"

#include "Net/UnrealNetwork.h"

AAtlantisPlayerState::AAtlantisPlayerState()
{
	// APlayerState already enables replication and marks itself always-relevant in its own
	// constructor, so there is no bReplicates to set here. It does however default to 1 Hz,
	// which is far too slow for a value like oxygen that the HUD reads continuously.
	SetNetUpdateFrequency(10.f);
	EquippedItems = TArray<FString>();
}

void AAtlantisPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAtlantisPlayerState, OxygenCapacity);
	DOREPLIFETIME(AAtlantisPlayerState, CurrentOxygen);
	DOREPLIFETIME(AAtlantisPlayerState, bBallastAllocation);
	DOREPLIFETIME(AAtlantisPlayerState, BallastState);
	DOREPLIFETIME(AAtlantisPlayerState, TraversalMode);
	DOREPLIFETIME(AAtlantisPlayerState, EquippedItems);
}

void AAtlantisPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	if (AAtlantisPlayerState* AtlantisPlayerState = Cast<AAtlantisPlayerState>(PlayerState))
	{
		AtlantisPlayerState->OxygenCapacity = OxygenCapacity;
		AtlantisPlayerState->CurrentOxygen = CurrentOxygen;
		AtlantisPlayerState->bBallastAllocation = bBallastAllocation;
		AtlantisPlayerState->BallastState = BallastState;
		AtlantisPlayerState->TraversalMode = TraversalMode;
		AtlantisPlayerState->EquippedItems = EquippedItems;
	}
}

void AAtlantisPlayerState::SetOxygenCapacity(const float NewOxygenCapacity)
{
	if (!HasAuthority() || FMath::IsNearlyEqual(OxygenCapacity, NewOxygenCapacity))
	{
		return;
	}

	const float OldOxygenCapacity = OxygenCapacity;
	OxygenCapacity = NewOxygenCapacity;

	// Replication never calls our RepNotify on the authority, so drive it by hand to keep
	// the listen-server host in step with every remote client.
	OnRep_OxygenCapacity(OldOxygenCapacity);
}

void AAtlantisPlayerState::SetCurrentOxygen(const float NewCurrentOxygen)
{
	if (!HasAuthority() || FMath::IsNearlyEqual(CurrentOxygen, NewCurrentOxygen))
	{
		return;
	}

	const float OldCurrentOxygen = CurrentOxygen;
	CurrentOxygen = NewCurrentOxygen;
	OnRep_CurrentOxygen(OldCurrentOxygen);
}

void AAtlantisPlayerState::SetBallastAllocated(const bool bNewBallastAllocation)
{
	if (!HasAuthority() || bBallastAllocation == bNewBallastAllocation)
	{
		return;
	}

	const bool bOldBallastAllocation = bBallastAllocation;
	bBallastAllocation = bNewBallastAllocation;

	OnRep_BallastAllocation(bOldBallastAllocation);
}

void AAtlantisPlayerState::SetBallastState(const EAtlantisBallastState NewBallastState)
{
	if (!HasAuthority() || BallastState == NewBallastState)
	{
		return;
	}

	const EAtlantisBallastState OldBallastState = BallastState;
	BallastState = NewBallastState;

	OnRep_BallastState(OldBallastState);
}

void AAtlantisPlayerState::SetTraversalMode(const int32 NewTraversalMode)
{
	if (!HasAuthority() || TraversalMode == NewTraversalMode)
	{
		return;
	}

	const int32 OldTraversalMode = TraversalMode;
	TraversalMode = NewTraversalMode;

	OnRep_TraversalMode(OldTraversalMode);
}

void AAtlantisPlayerState::OnRep_OxygenCapacity(const float OldOxygenCapacity) const
{
	OnOxygenCapacityChanged.Broadcast(OldOxygenCapacity, OxygenCapacity);
}

void AAtlantisPlayerState::OnRep_CurrentOxygen(const float OldCurrentOxygen) const
{
	OnCurrentOxygenChanged.Broadcast(OldCurrentOxygen, CurrentOxygen);
}

void AAtlantisPlayerState::OnRep_BallastAllocation(const bool bOldBallastAllocation)
{
	BroadcastBallastChanged();
}

void AAtlantisPlayerState::OnRep_BallastState(const EAtlantisBallastState OldBallastState)
{
	BroadcastBallastChanged();
}

void AAtlantisPlayerState::OnRep_TraversalMode(const int32 OldTraversalMode) const
{
	OnTraversalModeChanged.Broadcast(OldTraversalMode, TraversalMode);
}

void AAtlantisPlayerState::OnRep_EquippedItems(const TArray<FString>& OldEquippedItems) const
{
	OnEquippedItemsChanged.Broadcast(EquippedItems);
}

void AAtlantisPlayerState::BroadcastBallastChanged() const
{
	OnBallastChanged.Broadcast(bBallastAllocation, BallastState);
}

float AAtlantisPlayerState::GetRequiredBallastAllocation(const EAtlantisBallastState BallastState) const
{
	switch (BallastState)
	{
	case EAtlantisBallastState::Descend:
		return DescendOxygenAllocation;

	case EAtlantisBallastState::Wander:
		return WanderOxygenAllocation;

	case EAtlantisBallastState::Ascend:
		return AscendOxygenAllocation;


	case EAtlantisBallastState::None:
		//TODO: Confirm None case handling
		ensureMsgf(false, TEXT("GetRequiredBallastAllocation called with EValueState::None"));
		return 0;
	};
}

void AAtlantisPlayerState::RequestBallasteStateChange(const EAtlantisBallastState NewBallastState)
{


	//TODO confirm if the first initialization of the BallastState should pass through here.
	bool bOxygenAllocated = IsBallastAllocated();
	if (NewBallastState == EAtlantisBallastState::None) {
		//TODO something

		return;
	}
	if (NewBallastState == GetBallastState()) { return; }


	float RequiredAllocation = GetRequiredBallastAllocation(NewBallastState);
		//TODO confirm if current should be > or >= than required
		//TODO confirm that CurrentOxygen includes LockedOxygen

		if (GetCurrentOxygen() > RequiredAllocation) {
			

			//TODO confirm the use of lockeOxygen is ok
			//TODO confirm if Oxygen can be locked by other sources. If yes, confirm if the case BallastState = None must be handled here

			SetLockedOxygen(RequiredAllocation);
			SetBallastState(NewBallastState)
		}
}