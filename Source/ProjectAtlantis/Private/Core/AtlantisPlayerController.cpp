// Copyright (c) 2026 ASGC


#include "Core/AtlantisPlayerController.h"
#include "Core/AtlantisPlayerState.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PhysicsVolume.h"
#include "EnhancedInputComponent.h"

#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "HAL/PlatformProperties.h"

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

void AAtlantisPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (DescendBallastAction)
		{
			EnhancedInput->BindAction(DescendBallastAction, ETriggerEvent::Started, this, &AAtlantisPlayerController::SelectDescend);
		}
		if (WanderBallastAction)
		{
			EnhancedInput->BindAction(WanderBallastAction, ETriggerEvent::Started, this, &AAtlantisPlayerController::SelectWander);
		}
		if (AscendBallastAction)
		{
			EnhancedInput->BindAction(AscendBallastAction, ETriggerEvent::Started, this, &AAtlantisPlayerController::SelectAscend);
		}
	}
	RefreshMovementControls();
}

void AAtlantisPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	RefreshMovementControls();
}

void AAtlantisPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();
	RefreshMovementControls();
}

void AAtlantisPlayerController::RefreshMovementControls()
{
	if (const APawn* ControlledPawn = GetPawn())
	{
		const APhysicsVolume* Volume = ControlledPawn->GetPhysicsVolume();
		UpdateMovementControls(Volume && Volume->bWaterVolume);
	}
}

void AAtlantisPlayerController::UpdateMovementControls(bool bInWater)
{
	UInputMappingContext* ActiveContext = bInWater ? SwimmingMappingContext : DefaultMovementMappingContext;
	UInputMappingContext* InactiveContext = bInWater ? DefaultMovementMappingContext : SwimmingMappingContext;
	RemoveControls(InactiveContext);
	if (ActiveContext && !CurrentMappingContexts.Contains(ActiveContext))
	{
		AddControls(ActiveContext);
	}
}

void AAtlantisPlayerController::SelectDescend()
{
	RequestBallastState(EAtlantisBallastState::Descend);
}

void AAtlantisPlayerController::SelectWander()
{
	RequestBallastState(EAtlantisBallastState::Wander);
}

void AAtlantisPlayerController::SelectAscend()
{
	RequestBallastState(EAtlantisBallastState::Ascend);
}

void AAtlantisPlayerController::RequestBallastState(const EAtlantisBallastState NewBallastState)
{
	if (bControlsEnabled && GetPawn())
	{
		ServerSetBallastState(NewBallastState);
	}
}

void AAtlantisPlayerController::ServerSetBallastState_Implementation(const EAtlantisBallastState NewBallastState)
{
	if (!bControlsEnabled || !GetPawn()
		|| (NewBallastState != EAtlantisBallastState::Descend
			&& NewBallastState != EAtlantisBallastState::Wander
			&& NewBallastState != EAtlantisBallastState::Ascend))
	{
		return;
	}

	// PlayerState accepts the request only when the required oxygen can be reserved.
	if (AAtlantisPlayerState* AtlantisPlayerState = GetPlayerState<AAtlantisPlayerState>())
	{
		AtlantisPlayerState->SetBallastState(NewBallastState);
	}
}

UEnhancedInputLocalPlayerSubsystem* AAtlantisPlayerController::GetEnhancedInputSubsystem() const
{
	return ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
}

bool AAtlantisPlayerController::ShouldUseTouchControls() const
{
	const FString PlatformName(FPlatformProperties::PlatformName());
	const bool bIsMobilePlatform = PlatformName == TEXT("IOS") || PlatformName == TEXT("Android");
	return bIsMobilePlatform || bForceTouchControls;
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
	for (const UInputMappingContext* MappingContext : CurrentMappingContexts)
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
	for (const UInputMappingContext* MappingContext : CurrentMappingContexts)
	{
		if (MappingContext)
		{
			Subsystem->RemoveMappingContext(MappingContext, Options);
		}
	}
}
