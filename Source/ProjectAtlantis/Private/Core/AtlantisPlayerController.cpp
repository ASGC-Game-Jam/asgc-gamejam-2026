// Copyright (c) 2026 ASGC


#include "Core/AtlantisPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"

namespace
{
	/** Every context is applied at the same priority, matching the Blueprint this replaced. */
	constexpr int32 ControlMappingPriority = 0;

	/**
	 * The options the Blueprint passed to every Add/Remove Mapping Context call. These happen
	 * to be the FModifyContextOptions defaults, but they are spelled out so a future change to
	 * the engine defaults cannot quietly change our input behaviour.
	 */
	FModifyContextOptions MakeControlOptions()
	{
		FModifyContextOptions Options;
		Options.bIgnoreAllPressedKeysUntilRelease = true;
		Options.bForceImmediately = false;
		Options.bNotifyUserSettings = false;

		return Options;
	}
}

UEnhancedInputLocalPlayerSubsystem* AAtlantisPlayerController::GetEnhancedInputSubsystem() const
{
	return ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
}

void AAtlantisPlayerController::AddControls(UInputMappingContext* NewControlMappingContext)
{
	if (!NewControlMappingContext)
	{
		return;
	}

	CurrentMappingContexts.Add(NewControlMappingContext);
	OnControlsChanged.Broadcast();

	if (!bControlsEnabled)
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = GetEnhancedInputSubsystem())
	{
		Subsystem->AddMappingContext(NewControlMappingContext, ControlMappingPriority, MakeControlOptions());
	}
}

void AAtlantisPlayerController::RemoveControls(UInputMappingContext* MappingContext)
{
	if (!MappingContext)
	{
		return;
	}

	CurrentMappingContexts.Remove(MappingContext);
	OnControlsChanged.Broadcast();

	if (!bControlsEnabled)
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = GetEnhancedInputSubsystem())
	{
		Subsystem->RemoveMappingContext(MappingContext, MakeControlOptions());
	}
}

void AAtlantisPlayerController::ClearAllControls()
{
	// Withdraw from the subsystem BEFORE emptying the list. The Blueprint cleared the array
	// first and then iterated it, so the contexts were forgotten while staying applied.
	ClearMappingContexts();

	CurrentMappingContexts.Empty();
	OnControlsChanged.Broadcast();
}

void AAtlantisPlayerController::EnableAllControls()
{
	bControlsEnabled = true;
	EstablishMappingContexts();

	OnControlsEnabled.Broadcast();
}

void AAtlantisPlayerController::DisableAllControls()
{
	bControlsEnabled = false;
	ClearMappingContexts();

	OnControlsDisabled.Broadcast();
}

void AAtlantisPlayerController::EstablishMappingContexts()
{
	UEnhancedInputLocalPlayerSubsystem* Subsystem = GetEnhancedInputSubsystem();
	if (!Subsystem)
	{
		return;
	}

	const FModifyContextOptions Options = MakeControlOptions();
	for (UInputMappingContext* MappingContext : CurrentMappingContexts)
	{
		if (MappingContext)
		{
			Subsystem->AddMappingContext(MappingContext, ControlMappingPriority, Options);
		}
	}
}

void AAtlantisPlayerController::ClearMappingContexts()
{
	UEnhancedInputLocalPlayerSubsystem* Subsystem = GetEnhancedInputSubsystem();
	if (!Subsystem)
	{
		return;
	}

	const FModifyContextOptions Options = MakeControlOptions();
	for (UInputMappingContext* MappingContext : CurrentMappingContexts)
	{
		if (MappingContext)
		{
			Subsystem->RemoveMappingContext(MappingContext, Options);
		}
	}
}
