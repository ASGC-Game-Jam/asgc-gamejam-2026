// Copyright (c) 2026 ASGC


#include "Core/AtlantisPlayerState.h"

#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogAtlantisPlayerState, Log, All);

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
	DOREPLIFETIME(AAtlantisPlayerState, LockedOxygen);
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
		AtlantisPlayerState->LockedOxygen = LockedOxygen;
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

	OnRep_OxygenCapacity(OldOxygenCapacity);
}

void AAtlantisPlayerState::SetLockedOxygen(const float NewLockedOxygen)
{
	if (!HasAuthority() || FMath::IsNearlyEqual(LockedOxygen, NewLockedOxygen))
	{
		return;
	}

	const float OldLockedOxygen = LockedOxygen;
	LockedOxygen = NewLockedOxygen;

	OnRep_LockedOxygen(OldLockedOxygen);
}

void AAtlantisPlayerState::SetCurrentOxygen(const float NewCurrentOxygen)
{
	// Current oxygen includes the ballast allocation; consumption can only use the unlocked portion.
	const float ClampedOxygen = FMath::Max(NewCurrentOxygen, LockedOxygen);
	if (!HasAuthority() || FMath::IsNearlyEqual(CurrentOxygen, ClampedOxygen))
	{
		return;
	}

	const float OldCurrentOxygen = CurrentOxygen;
	CurrentOxygen = ClampedOxygen;
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

	const float RequiredAllocation = GetRequiredBallastAllocation(NewBallastState);
	if (RequiredAllocation < 0.f)
	{
		UE_LOG(LogAtlantisPlayerState, Fatal, TEXT("Ballast state %d has an invalid oxygen allocation: %f"),
		       static_cast<uint8>(NewBallastState), RequiredAllocation);
		return;
	}
	else if (CurrentOxygen <= RequiredAllocation)
	{
		return;
	}

	const EAtlantisBallastState OldBallastState = BallastState;
	BallastState = NewBallastState;
	SetLockedOxygen(RequiredAllocation);
	bBallastAllocation = RequiredAllocation > 0.f;

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

void AAtlantisPlayerState::OnRep_LockedOxygen(const float OldLockedOxygen) const
{
	OnLockedOxygenChanged.Broadcast(OldLockedOxygen, LockedOxygen);
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

float AAtlantisPlayerState::GetRequiredBallastAllocation(const EAtlantisBallastState RequiredBallastState) const
{
	switch (RequiredBallastState)
	{
		case EAtlantisBallastState::Descend:
		{
			return DescendOxygenAllocation;
		}
		case EAtlantisBallastState::Wander:
		{
			return WanderOxygenAllocation;
		}
		case EAtlantisBallastState::Ascend:
		{
			return AscendOxygenAllocation;
		}
		case EAtlantisBallastState::None:
		default:
		{
			UE_LOG(LogAtlantisPlayerState, Error, TEXT("Invalid ballast state: %d"), static_cast<uint8>(RequiredBallastState));
			return -1.f;
		}
	}
}
