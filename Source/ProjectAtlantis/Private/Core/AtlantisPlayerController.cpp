// Copyright (c) 2026 ASGC


#include "Core/AtlantisPlayerController.h"
#include "Core/AtlantisPlayerState.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PhysicsVolume.h"
#include "EnhancedInputComponent.h"

#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "TimerManager.h"
#include "HAL/PlatformProperties.h"
#include "Debug/AtlantisCheatManager.h"
#include "Debug/AtlantisDebugMenu.h"
#include "Debug/AtlantisDebugMenuSettings.h"
#include "Kismet/GameplayStatics.h"

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

void AAtlantisPlayerController::RequestDebugRestart()
{
#if !UE_BUILD_SHIPPING
	if (!IsLocalController() || GetNetMode() != NM_Standalone
		|| (!DebugMenu && (!bControlsEnabled || bShowMouseCursor)))
	{
		return;
	}
	CloseDebugMenu();
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
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
		EnhancedInput->BindAction(DebugInput.Reset, ETriggerEvent::Triggered, this, &AAtlantisPlayerController::RequestDebugRestart);
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
		// ADDED: one key that steps through the ballast states (Descend -> Wander -> Ascend -> Descend).
		if (CycleBallastAction)
		{
			EnhancedInput->BindAction(CycleBallastAction, ETriggerEvent::Started, this,
				&AAtlantisPlayerController::CycleBallastState);
		}
		// Steps the other way (Descend -> Ascend -> Wander -> Descend).
		if (CycleBallastBackAction)
		{
			EnhancedInput->BindAction(CycleBallastBackAction, ETriggerEvent::Started, this,
				&AAtlantisPlayerController::CycleBallastBackState);
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

// ADDED: steps the *pending* ballast state forward one step per press. The real state is only
// applied (via RequestBallastState) once BallastCommitDelay seconds pass without another press.
void AAtlantisPlayerController::CycleBallastState(const FInputActionValue& ActionValue)
{
	if (!bControlsEnabled || !GetPawn())
	{
		return;
	}

	const AAtlantisPlayerState* State = GetPlayerState<AAtlantisPlayerState>();
	if (!State)
	{
		return;
	}

	// GetBallastState() is a guess, check the real getter name in AtlantisPlayerState.h
	const EAtlantisBallastState From =
		PendingBallastState.IsSet() ? PendingBallastState.GetValue() : State->GetBallastState();

	EAtlantisBallastState Next = EAtlantisBallastState::Descend;
	switch (From)
	{
	case EAtlantisBallastState::Descend: Next = EAtlantisBallastState::Wander; break;
	case EAtlantisBallastState::Wander:  Next = EAtlantisBallastState::Ascend; break;
	default:                             Next = EAtlantisBallastState::Descend; break;
	}
	PendingBallastState = Next;

	// Setting the timer again restarts it, so each press extends the window
	GetWorldTimerManager().SetTimer(BallastCommitTimer, this,
		&AAtlantisPlayerController::CommitPendingBallastState, BallastCommitDelay, false);
}

// Same as CycleBallastState, but steps the pending state backwards.
void AAtlantisPlayerController::CycleBallastBackState(const FInputActionValue& ActionValue)
{
	if (!bControlsEnabled || !GetPawn())
	{
		return;
	}

	const AAtlantisPlayerState* State = GetPlayerState<AAtlantisPlayerState>();
	if (!State)
	{
		return;
	}

	const EAtlantisBallastState From =
		PendingBallastState.IsSet() ? PendingBallastState.GetValue() : State->GetBallastState();

	EAtlantisBallastState Next = EAtlantisBallastState::Wander;
	switch (From)
	{
	case EAtlantisBallastState::Descend: Next = EAtlantisBallastState::Ascend; break;
	case EAtlantisBallastState::Wander:  Next = EAtlantisBallastState::Descend; break;
	default:                             Next = EAtlantisBallastState::Wander; break;
	}
	PendingBallastState = Next;

	GetWorldTimerManager().SetTimer(BallastCommitTimer, this,
		&AAtlantisPlayerController::CommitPendingBallastState, BallastCommitDelay, false);
}

// ADDED: applies the pending state once the player has stopped cycling.
void AAtlantisPlayerController::CommitPendingBallastState()
{
	if (PendingBallastState.IsSet())
	{
		const EAtlantisBallastState ToApply = PendingBallastState.GetValue();
		PendingBallastState.Reset();
		RequestBallastState(FInputActionValue(), ToApply);
	}
}

void AAtlantisPlayerController::ServerSetBallastState_Implementation(const EAtlantisBallastState NewBallastState)
{
	if (!bControlsEnabled || !GetPawn() || NewBallastState == EAtlantisBallastState::None)
	{
		return;
	}

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

	// ADDED: drop any half-finished ballast cycle so it can't fire while controls are off.
	GetWorldTimerManager().ClearTimer(BallastCommitTimer);
	PendingBallastState.Reset();

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