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
	DOREPLIFETIME(AAtlantisPlayerState, BallastState);
	DOREPLIFETIME(AAtlantisPlayerState, TraversalMode);
	DOREPLIFETIME(AAtlantisPlayerState, EquippedItems);
	DOREPLIFETIME(AAtlantisPlayerState, bCriticalOxygen);
}

void AAtlantisPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	if (AAtlantisPlayerState* AtlantisPlayerState = Cast<AAtlantisPlayerState>(PlayerState))
	{
		AtlantisPlayerState->OxygenCapacity = OxygenCapacity;
		AtlantisPlayerState->LockedOxygen = LockedOxygen;
		AtlantisPlayerState->CurrentOxygen = CurrentOxygen;
		AtlantisPlayerState->BallastState = BallastState;
		AtlantisPlayerState->TraversalMode = TraversalMode;
		AtlantisPlayerState->EquippedItems = EquippedItems;
		AtlantisPlayerState->bCriticalOxygen = bCriticalOxygen;
	}
}

void AAtlantisPlayerState::BeginPlay()
{
	Super::BeginPlay();
	// CriticalOxygen flag is updated on oxygen level changes (both current and locked) through setters.
	// If default oxygen level is critical the flag doesn't reflect that until next oxygen level change.
	if (HasAuthority())
	{
		UpdateCriticalOxygen();
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
	UpdateCriticalOxygen();
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
	UpdateCriticalOxygen();
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

void AAtlantisPlayerState::SetCriticalOxygen(bool bNewCriticalOxygen)
{
	if (!HasAuthority() || bCriticalOxygen == bNewCriticalOxygen)
	{
		return;
	}

	bCriticalOxygen = bNewCriticalOxygen;
	OnRep_CriticalOxygen();
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

void AAtlantisPlayerState::OnRep_BallastState(const EAtlantisBallastState OldBallastState) const
{
	OnBallastChanged.Broadcast(IsBallastAllocated(), BallastState);
}

void AAtlantisPlayerState::OnRep_TraversalMode(const int32 OldTraversalMode) const
{
	OnTraversalModeChanged.Broadcast(OldTraversalMode, TraversalMode);
}

void AAtlantisPlayerState::OnRep_EquippedItems(const TArray<FString>& OldEquippedItems) const
{
	OnEquippedItemsChanged.Broadcast(EquippedItems);
}

void AAtlantisPlayerState::OnRep_CriticalOxygen() const
{
	OnCriticalOxygenChanged.Broadcast(bCriticalOxygen);
}

void AAtlantisPlayerState::UpdateCriticalOxygen()
{
	const float AvailableOxygen = CurrentOxygen - LockedOxygen;
	if (!bCriticalOxygen && AvailableOxygen <= CriticalOxygenThreshold)
	{
		SetCriticalOxygen(true);
	}
	else if (bCriticalOxygen && AvailableOxygen > CriticalOxygenThreshold + CriticalOxygenRecoveryMargin)
	{
		SetCriticalOxygen(false);
	}
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
			UE_LOG(LogAtlantisPlayerState, Fatal, TEXT("Invalid ballast state: %d"),
			       static_cast<uint8>(RequiredBallastState));
			return -1.f;
		}
	}
}

bool AAtlantisPlayerState::RequestOxygen(const float RequestedOxygen)
{
	if (!HasAuthority() || RequestedOxygen < 0.f)
	{
		return false;
	}
	const bool bRequestAccepted = GetAvailableOxygen() >= RequestedOxygen;

	if (bRequestAccepted)
	{
		SetCurrentOxygen(CurrentOxygen - RequestedOxygen);
	}

	return bRequestAccepted;
}

bool AAtlantisPlayerState::RequestOxygenAllocation(const float RequestedOxygen)
{
	if (!HasAuthority() || RequestedOxygen < 0.f)
	{
		return false;
	}
	const bool bRequestAccepted = GetAvailableOxygen() >= RequestedOxygen;

	if (bRequestAccepted)
	{
		SetLockedOxygen(LockedOxygen + RequestedOxygen);
	}

	return bRequestAccepted;
}

bool AAtlantisPlayerState::ReleaseOxygen(const float ReleasedOxygen)
{
	if (!HasAuthority() || ReleasedOxygen < 0.f)
	{
		return false;
	}
	const bool bRequestAccepted = LockedOxygen >= ReleasedOxygen;

	if (bRequestAccepted)
	{
		SetLockedOxygen(LockedOxygen - ReleasedOxygen);
	}

	return bRequestAccepted;
}
