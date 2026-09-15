// Copyright (c) 2026 ASGC


#include "Core/AtlantisPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"

// Anonymous namespace: everything inside has internal linkage, so these helpers are private to
// this .cpp. Another file can define its own MakeControlOptions without a duplicate-symbol link
// error, and nothing outside this file can come to depend on them. Prefer it over marking each
// function `static`: one block covers functions, constants, and helper types alike, and `static`
// cannot be applied to a type at all.
//
// Unreal caveat: unity builds paste several .cpp files from a module into one translation unit,
// where their anonymous namespaces merge into one. Two files defining the same helper name will
// then collide at compile time, and because adaptive unity compiles files you are editing
// separately, that often only surfaces on a clean or CI build. Keep names here file-specific.
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
