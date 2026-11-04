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
#include "Core/AtlantisPlayerCharacter.h"
#include "Debug/AtlantisCheatManager.h"
#include "Debug/AtlantisDebugMenu.h"
#include "Debug/AtlantisDebugMenuSettings.h"

AAtlantisPlayerController::AAtlantisPlayerController()
{
	CheatClass = UAtlantisCheatManager::StaticClass();
}

void AAtlantisPlayerController::AddCheats(bool bForce)
{
	// Existing controller Blueprints serialize the old engine CheatManager default.
	// Upgrade only that default, preserving explicitly configured project subclasses.
	if (CheatClass == UCheatManager::StaticClass())
	{
		CheatClass = UAtlantisCheatManager::StaticClass();
	}
	Super::AddCheats(bForce);
}

void AAtlantisPlayerController::ToggleDebugMenu()
{
#if !UE_BUILD_SHIPPING
	if (!IsLocalController())
	{
		return;
	}
	if (DebugMenu)
	{
		DebugMenu->RemoveFromParent();
		DebugMenu = nullptr;
		SetIgnoreMoveInput(false);
		SetIgnoreLookInput(false);
		bShowMouseCursor = false;
		SetInputMode(FInputModeGameOnly());
		EnableAllControls();
		RefreshDebugInputContext();
		return;
	}
	// Do not take input ownership from another menu.
	if (bShowMouseCursor || !bControlsEnabled || !GetPawn())
	{
		return;
	}
	EnableCheats();
	if (!Cast<UAtlantisCheatManager>(CheatManager))
	{
		return;
	}
	DebugMenu = CreateWidget<UAtlantisDebugMenu>(this);
	if (!DebugMenu)
	{
		return;
	}
	if (!DebugMenu->AddToPlayerScreen(10000))
	{
		DebugMenu = nullptr;
		return;
	}
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
	DisableAllControls();
	RefreshDebugInputContext();
	bShowMouseCursor = true;
	FInputModeGameAndUI Mode;
	Mode.SetWidgetToFocus(DebugMenu->TakeWidget());
	Mode.SetHideCursorDuringCapture(false);
	SetInputMode(Mode);
	DebugMenu->FocusFirstAction();
#endif
}

void AAtlantisPlayerController::RequestDebugReset()
{
#if !UE_BUILD_SHIPPING
	if (IsLocalController() && (DebugMenu || (bControlsEnabled && !bShowMouseCursor)))
	{
		if (HasAuthority())
		{
			ServerDebugReset_Implementation();
		}
		else
		{
			ServerDebugReset();
		}
	}
#endif
}

void AAtlantisPlayerController::ServerDebugReset_Implementation()
{
#if !UE_BUILD_SHIPPING
	if (AAtlantisPlayerCharacter* PlayerCharacter = Cast<AAtlantisPlayerCharacter>(GetPawn()))
	{
		PlayerCharacter->MoveToStart();
		ClientSetRotation(GetControlRotation(), true);
	}
#endif
}

void AAtlantisPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = GetEnhancedInputSubsystem())
	{
		if (DebugGameplayMappingContext)
		{
			Subsystem->RemoveMappingContext(DebugGameplayMappingContext);
		}
		if (DebugMenuMappingContext)
		{
			Subsystem->RemoveMappingContext(DebugMenuMappingContext);
		}
	}
	if (DebugMenu)
	{
		DebugMenu->RemoveFromParent();
		DebugMenu = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

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
#if !UE_BUILD_SHIPPING
		const FAtlantisDebugInputMappings DebugInput = GetDefault<UAtlantisDebugMenuSettings>()->CreateInputMappings(this);
		DebugGameplayMappingContext = DebugInput.Gameplay;
		DebugMenuMappingContext = DebugInput.Menu;
		EnhancedInput->BindAction(DebugInput.Toggle, ETriggerEvent::Triggered, this, &AAtlantisPlayerController::ToggleDebugMenu);
		EnhancedInput->BindAction(DebugInput.GamepadToggle, ETriggerEvent::Triggered, this, &AAtlantisPlayerController::ToggleDebugMenu);
		EnhancedInput->BindAction(DebugInput.Reset, ETriggerEvent::Triggered, this, &AAtlantisPlayerController::RequestDebugReset);
		EnhancedInput->BindAction(DebugInput.Close, ETriggerEvent::Triggered, this, &AAtlantisPlayerController::CloseDebugMenu);
		RefreshDebugInputContext();
#endif
		if (DescendBallastAction)
		{
			EnhancedInput->BindAction(DescendBallastAction, ETriggerEvent::Started, this,
			                          &AAtlantisPlayerController::RequestBallastState, EAtlantisBallastState::Descend);
		}
		if (WanderBallastAction)
		{
			EnhancedInput->BindAction(WanderBallastAction, ETriggerEvent::Started, this,
			                          &AAtlantisPlayerController::RequestBallastState, EAtlantisBallastState::Wander);
		}
		if (AscendBallastAction)
		{
			EnhancedInput->BindAction(AscendBallastAction, ETriggerEvent::Started, this,
			                          &AAtlantisPlayerController::RequestBallastState, EAtlantisBallastState::Ascend);
		}
	}
	if (BallastMappingContext && !CurrentMappingContexts.Contains(BallastMappingContext))
	{
		AddControls(BallastMappingContext);
	}
	
	RefreshMovementControls();
}

void AAtlantisPlayerController::RefreshDebugInputContext()
{
#if !UE_BUILD_SHIPPING
	UEnhancedInputLocalPlayerSubsystem* Subsystem = GetEnhancedInputSubsystem();
	if (!Subsystem || !DebugGameplayMappingContext || !DebugMenuMappingContext)
	{
		return;
	}
	const FModifyContextOptions Options = MakeControlOptions();
	Subsystem->RemoveMappingContext(DebugGameplayMappingContext, Options);
	Subsystem->RemoveMappingContext(DebugMenuMappingContext, Options);
	Subsystem->AddMappingContext(DebugMenu ? DebugMenuMappingContext : DebugGameplayMappingContext, 1000, Options);
#endif
}

void AAtlantisPlayerController::CloseDebugMenu()
{
	if (DebugMenu)
	{
		ToggleDebugMenu();
	}
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

void AAtlantisPlayerController::RequestBallastState(const FInputActionValue& ActionValue, const EAtlantisBallastState NewBallastState)
{
	if (bControlsEnabled && GetPawn())
	{
		ServerSetBallastState(NewBallastState);
	}
}

void AAtlantisPlayerController::ServerSetBallastState_Implementation(const EAtlantisBallastState NewBallastState)
{
	if (!bControlsEnabled || !GetPawn() || NewBallastState == EAtlantisBallastState::None)
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

void AAtlantisPlayerController::RestrictControls(FName Requester, UInputMappingContext* MappingContext)
{
	if (Requester.IsNone() || !IsValid(MappingContext))
	{
		return;
	}

	TSet<FName>& Restrictions = InputRestrictions.FindOrAdd(MappingContext);

	const bool bWasUnrestricted = Restrictions.IsEmpty();
	Restrictions.Add(Requester);

	if (bWasUnrestricted)
	{
		RemoveControls(MappingContext);
	}
}
void AAtlantisPlayerController::UnrestrictControls(FName Requester, UInputMappingContext* MappingContext)
{
	if (Requester.IsNone() || !IsValid(MappingContext))
	{
		return;
	}

	TSet<FName>* Restrictions = InputRestrictions.Find(MappingContext);

	if (!Restrictions)
	{
		return;
	}

	Restrictions->Remove(Requester);

	if (Restrictions->IsEmpty())
	{
		AddControls(MappingContext);
		InputRestrictions.Remove(MappingContext);
	}
}
