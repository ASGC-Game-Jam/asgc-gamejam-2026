// Copyright (c) 2026 ASGC


#include "Core/AtlantisPlayerState.h"
#include "Core/AtlantisOxygenComponent.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogAtlantisPlayerState, Log, All);

void AAtlantisPlayerState::BeginPlay()
{
	Super::BeginPlay();
	RefreshOxygenConsumption();
}

AAtlantisPlayerState::AAtlantisPlayerState()
{
	// APlayerState already enables replication and marks itself always-relevant in its own
	// constructor, so there is no bReplicates to set here. It does however default to 1 Hz,
	// which is far too slow for a value like oxygen that the HUD reads continuously.
	SetNetUpdateFrequency(10.f);
	OxygenComponent = CreateDefaultSubobject<UAtlantisOxygenComponent>(TEXT("OxygenComponent"));
	EquippedItems = TArray<FString>();
}

void AAtlantisPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAtlantisPlayerState, BallastState);
	DOREPLIFETIME(AAtlantisPlayerState, TraversalMode);
	DOREPLIFETIME(AAtlantisPlayerState, EquippedItems);
}

void AAtlantisPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	if (AAtlantisPlayerState* AtlantisPlayerState = Cast<AAtlantisPlayerState>(PlayerState))
	{
		AtlantisPlayerState->BallastState = BallastState;
		AtlantisPlayerState->TraversalMode = TraversalMode;
		AtlantisPlayerState->EquippedItems = EquippedItems;
		if (OxygenComponent && AtlantisPlayerState->OxygenComponent)
		{
			OxygenComponent->CopyStateTo(*AtlantisPlayerState->OxygenComponent);
		}
	}
}


void AAtlantisPlayerState::SetBallastState(const EAtlantisBallastState NewBallastState)
{
	if (!HasAuthority() || BallastState == NewBallastState || !OxygenComponent)
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
	else if (OxygenComponent->GetCurrentOxygen() <= RequiredAllocation)
	{
		return;
	}

	if (!OxygenComponent->ReallocateOxygen(RequiredAllocation))
	{
		return;
	}

	const EAtlantisBallastState OldBallastState = BallastState;
	BallastState = NewBallastState;

	OnRep_BallastState(OldBallastState);
}

void AAtlantisPlayerState::SetTraversalMode(const EAtlantisTraversalMode NewTraversalMode)
{
	if (!HasAuthority() || TraversalMode == NewTraversalMode)
	{
		return;
	}

	const EAtlantisTraversalMode OldTraversalMode = TraversalMode;
	TraversalMode = NewTraversalMode;
	OnRep_TraversalMode(OldTraversalMode);
	RefreshOxygenConsumption();
}

void AAtlantisPlayerState::OnRep_BallastState(const EAtlantisBallastState OldBallastState) const
{
	OnBallastChanged.Broadcast(IsBallastAllocated(), BallastState);
}

void AAtlantisPlayerState::OnRep_TraversalMode(const EAtlantisTraversalMode OldTraversalMode) const
{
	OnTraversalModeChanged.Broadcast(OldTraversalMode, TraversalMode);
}

void AAtlantisPlayerState::OnRep_EquippedItems(const TArray<FString>& OldEquippedItems) const
{
	OnEquippedItemsChanged.Broadcast(EquippedItems);
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

bool AAtlantisPlayerState::IsBallastAllocated() const
{
	return OxygenComponent && OxygenComponent->GetAllocatedOxygen() > 0.f;
}

bool AAtlantisPlayerState::IsUsingOxygen() const
{
	return TraversalMode == EAtlantisTraversalMode::Swimming || TraversalMode == EAtlantisTraversalMode::SurfaceWalking;
}

void AAtlantisPlayerState::RefreshOxygenConsumption() const
{
	if (!HasAuthority() || !OxygenComponent)
	{
		return;
	}

	OxygenComponent->SetConsumingOxygen(IsUsingOxygen());
}
