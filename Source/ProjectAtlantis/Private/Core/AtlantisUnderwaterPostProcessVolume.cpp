// Copyright (c) 2026 ASGC

#include "Core/AtlantisUnderwaterPostProcessVolume.h"

#include "Core/AtlantisPlayerState.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

void AAtlantisUnderwaterPostProcessVolume::BeginPlay()
{
	Super::BeginPlay();

	ApplyTraversalMode(EAtlantisTraversalMode::None);
	TryBindToPlayerState();
}

void AAtlantisUnderwaterPostProcessVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}

	if (AAtlantisPlayerState* PlayerState = BoundPlayerState.Get())
	{
		PlayerState->OnTraversalModeChanged.RemoveDynamic(this, &ThisClass::HandleTraversalModeChanged);
	}

	BoundPlayerState.Reset();

	Super::EndPlay(EndPlayReason);
}

void AAtlantisUnderwaterPostProcessVolume::TryBindToPlayerState()
{
	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	const APlayerController* PlayerController = World->GetFirstPlayerController();
	AAtlantisPlayerState* PlayerState = PlayerController
		                                    ? PlayerController->GetPlayerState<AAtlantisPlayerState>()
		                                    : nullptr;


	if (!PlayerState)
	{
		//In case the player state does not exist before this volume begins play
		World->GetTimerManager().SetTimerForNextTick(this, &ThisClass::TryBindToPlayerState);
		return;
	}

	PlayerState->OnTraversalModeChanged.AddUniqueDynamic(this, &ThisClass::HandleTraversalModeChanged);
	BoundPlayerState = PlayerState;

	ApplyTraversalMode(PlayerState->GetTraversalMode());
}

void AAtlantisUnderwaterPostProcessVolume::HandleTraversalModeChanged(EAtlantisTraversalMode /*OldTraversalMode*/,
                                                                      EAtlantisTraversalMode NewTraversalMode)
{
	ApplyTraversalMode(NewTraversalMode);
}

void AAtlantisUnderwaterPostProcessVolume::ApplyTraversalMode(EAtlantisTraversalMode TraversalMode)
{
	switch (TraversalMode)
	{
	case EAtlantisTraversalMode::Swimming:
	case EAtlantisTraversalMode::SurfaceWalking:
		{
			bEnabled = true;
			break;
		}
	case EAtlantisTraversalMode::Terrestrial:
	case EAtlantisTraversalMode::None:
	default:
		{
			bEnabled = false;
			break;
		}
	}
}
