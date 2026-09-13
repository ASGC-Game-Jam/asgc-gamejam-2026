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

void AAtlantisPlayerState::SetBallastState(const int32 NewBallastState)
{
	if (!HasAuthority() || BallastState == NewBallastState)
	{
		return;
	}

	const int32 OldBallastState = BallastState;
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

void AAtlantisPlayerState::OnRep_BallastAllocation(const bool bOldBallastAllocation)
{
	BroadcastBallastChanged();
}

void AAtlantisPlayerState::OnRep_BallastState(const int32 OldBallastState)
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
