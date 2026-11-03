// Copyright (c) 2026 ASGC


#include "Core/AtlantisPlayerState.h"
#include "Core/AtlantisOxygenComponent.h"
#include "Analytics/GameplayAnalyticsSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogAtlantisPlayerState, Log, All);

void AAtlantisPlayerState::BeginPlay()
{
	Super::BeginPlay();
	OnPawnSet.AddDynamic(this, &AAtlantisPlayerState::HandlePawnSet);
	RefreshOxygenConsumption();

	if (!HasAuthority())
	{
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		GameplayAnalyticsSubsystem = GameInstance->GetSubsystem<UGameplayAnalyticsSubsystem>();
		if (UGameplayAnalyticsSubsystem* Analytics = GameplayAnalyticsSubsystem.Get())
		{
			Analytics->RegisterMetric(TEXT("Ballast:Duration:Descend"), EGameplayAnalyticsAggregation::Sum);
			Analytics->RegisterMetric(TEXT("Ballast:Duration:Wander"), EGameplayAnalyticsAggregation::Sum);
			Analytics->RegisterMetric(TEXT("Ballast:Duration:Ascend"), EGameplayAnalyticsAggregation::Sum);
			ActiveGameplayTickHandle = Analytics->GetOnActiveGameplayTick().AddUObject(
				this, &AAtlantisPlayerState::HandleActiveGameplayTick);
		}
	}
}

void AAtlantisPlayerState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGameplayAnalyticsSubsystem* Analytics = GameplayAnalyticsSubsystem.Get())
	{
		if (ActiveGameplayTickHandle.IsValid())
		{
			Analytics->GetOnActiveGameplayTick().Remove(ActiveGameplayTickHandle);
			ActiveGameplayTickHandle.Reset();
		}
	}
	GameplayAnalyticsSubsystem.Reset();
	Super::EndPlay(EndPlayReason);
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

void AAtlantisPlayerState::HandlePawnSet(APlayerState* PlayerState, APawn* NewPawn, APawn* OldPawn)
{
	RefreshOxygenConsumption();
}

void AAtlantisPlayerState::HandleActiveGameplayTick(UWorld* World, const double ActiveDeltaSeconds)
{
	if (!HasAuthority() || World != GetWorld() || ActiveDeltaSeconds <= 0.0 || !FMath::IsFinite(ActiveDeltaSeconds))
	{
		return;
	}

	const TCHAR* MetricId = nullptr;
	switch (BallastState)
	{
	case EAtlantisBallastState::Descend:
		MetricId = TEXT("Ballast:Duration:Descend");
		break;
	case EAtlantisBallastState::Wander:
		MetricId = TEXT("Ballast:Duration:Wander");
		break;
	case EAtlantisBallastState::Ascend:
		MetricId = TEXT("Ballast:Duration:Ascend");
		break;
	default:
		return;
	}

	if (UGameplayAnalyticsSubsystem* Analytics = GameplayAnalyticsSubsystem.Get())
	{
		Analytics->RecordValue(MetricId, static_cast<float>(ActiveDeltaSeconds));
	}
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

	// Count only accepted, authoritative state changes. Initial and replicated
	// notifications are deliberately excluded from analytics.
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UGameplayAnalyticsSubsystem* Analytics = GameInstance->GetSubsystem<UGameplayAnalyticsSubsystem>())
			{
				const TCHAR* MetricId = nullptr;
				switch (NewBallastState)
				{
				case EAtlantisBallastState::Descend:
				{
					MetricId = TEXT("Ballast:Entered:Descend");
					break;
				}
				case EAtlantisBallastState::Wander:
				{
					MetricId = TEXT("Ballast:Entered:Wander");
					break;
				}
				case EAtlantisBallastState::Ascend:
				{
					MetricId = TEXT("Ballast:Entered:Ascend");
					break;
				}
				default: break;
				}
				if (MetricId)
				{
					Analytics->RegisterMetric(MetricId, EGameplayAnalyticsAggregation::Count);
					Analytics->RecordEvent(MetricId);
				}
			}
		}
	}
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
	const ACharacter* Character = GetPawn<ACharacter>();
	return Character && Character->GetCharacterMovement()->IsInWater();
}

void AAtlantisPlayerState::RefreshOxygenConsumption() const
{
	if (!HasAuthority() || !OxygenComponent)
	{
		return;
	}

	OxygenComponent->SetConsumingOxygen(IsUsingOxygen());
}
