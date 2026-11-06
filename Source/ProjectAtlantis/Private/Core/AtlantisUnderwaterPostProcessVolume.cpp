// Copyright (c) 2026 ASGC

#include "Core/AtlantisUnderwaterPostProcessVolume.h"

#include "Core/AtlantisMovementComponent.h"
#include "Core/AtlantisPlayerState.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

void AAtlantisUnderwaterPostProcessVolume::BeginPlay()
{
	Super::BeginPlay();

	bEnabled = false;
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
		PlayerState->OnPawnSet.RemoveDynamic(this, &ThisClass::HandlePawnSet);
		UnbindFromPawn(PlayerState->GetPawn());
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

	PlayerState->OnPawnSet.AddUniqueDynamic(this, &ThisClass::HandlePawnSet);
	BoundPlayerState = PlayerState;

	BindToPawn(PlayerState->GetPawn());
}

void AAtlantisUnderwaterPostProcessVolume::HandlePawnSet(APlayerState* /*Player*/, APawn* NewPawn, APawn* OldPawn)
{
	UnbindFromPawn(OldPawn);
	BindToPawn(NewPawn);
}

void AAtlantisUnderwaterPostProcessVolume::BindToPawn(APawn* Pawn)
{
	UAtlantisMovementComponent* Movement = Pawn ? Pawn->FindComponentByClass<UAtlantisMovementComponent>() : nullptr;

	if (!Movement)
	{
		bEnabled = false;
		return;
	}

	Movement->OnUnderwaterChanged.AddUniqueDynamic(this, &ThisClass::HandleUnderwaterChanged);

	bEnabled = Movement->IsInWater();
}

void AAtlantisUnderwaterPostProcessVolume::UnbindFromPawn(APawn* Pawn)
{
	if (UAtlantisMovementComponent* Movement =
		Pawn ? Pawn->FindComponentByClass<UAtlantisMovementComponent>() : nullptr)
	{
		Movement->OnUnderwaterChanged.RemoveDynamic(this, &ThisClass::HandleUnderwaterChanged);
	}
}

void AAtlantisUnderwaterPostProcessVolume::HandleUnderwaterChanged(bool bUnderwater)
{
	bEnabled = bUnderwater;
}
