// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AtlantisPlayerController.generated.h"

struct FInputActionValue;
class UInputMappingContext;
class UInputAction;
class UEnhancedInputLocalPlayerSubsystem;
enum class EAtlantisBallastState : uint8;

/** The desired set of control mapping contexts changed. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnControlsChanged);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInputDeviceChanged, bool, bUsingGamepad);

/** Controls were re-applied to the input subsystem as a whole. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnControlsEnabled);

/** Controls were withdrawn from the input subsystem as a whole. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnControlsDisabled);

/**
 * Owns the player's Enhanced Input mapping contexts.
 *
 * `CurrentMappingContexts` is the *desired* set of controls; `bControlsEnabled` decides whether
 * that set is currently pushed to the Enhanced Input subsystem. Keeping the two apart lets
 * gameplay add and remove controls while input is suppressed — a cutscene, a menu — and have
 * the correct set restored when control returns, without every caller having to remember what
 * was active.
 *
 * Change events follow the project fan-out convention; see Docs/Replicated State Pattern.md.
 */
UCLASS(Config = Game)
class PROJECTATLANTIS_API AAtlantisPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual bool InputKey(const FInputKeyEventArgs& Params) override;

	UPROPERTY(BlueprintAssignable, Category = "Atlantis|Controls")
	FOnInputDeviceChanged OnInputDeviceChanged;

	UFUNCTION(BlueprintPure, Category = "Atlantis|Controls")
	bool IsUsingGamepad() const { return bUsingGamepad; }

	void UpdateMovementControls(bool bInWater);
	virtual void OnRep_Pawn() override;

	//~ Change events. Bind from UI rather than polling.
	UPROPERTY(BlueprintAssignable, Category = "Atlantis|Controls")
	FOnControlsChanged OnControlsChanged;

	UPROPERTY(BlueprintAssignable, Category = "Atlantis|Controls")
	FOnControlsEnabled OnControlsEnabled;

	UPROPERTY(BlueprintAssignable, Category = "Atlantis|Controls")
	FOnControlsDisabled OnControlsDisabled;

	/** Adds a context to the desired set, applying it now if controls are enabled. */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|Controls")
	void AddControls(UInputMappingContext* NewControlMappingContext);

	/** Drops a context from the desired set, withdrawing it now if controls are enabled. */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|Controls")
	void RemoveControls(UInputMappingContext* MappingContext);

	/** Withdraws every context and forgets the desired set entirely. */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|Controls")
	void ClearAllControls();

	/** Re-applies the desired set to the input subsystem. */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|Controls")
	void EnableAllControls();

	/** Withdraws the desired set from the input subsystem without forgetting it. */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|Controls")
	void DisableAllControls();

	UFUNCTION(BlueprintPure, Category = "Atlantis|Controls")
	bool AreControlsEnabled() const { return bControlsEnabled; }

	/** Whether UMG touch controls should be shown on this platform or by configuration. */
	UFUNCTION(BlueprintPure, Category = "Atlantis|Controls", meta = (DisplayName = "Should Use Touch Controls"))
	bool ShouldUseTouchControls() const;

	/** Explicitly enables touch controls on non-mobile platforms when set in project config. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Atlantis|Controls", meta = (DisplayName = "Force Touch Controls"))
	bool bForceTouchControls = false;

	UFUNCTION(BlueprintPure, Category = "Atlantis|Controls")
	const TArray<UInputMappingContext*>& GetCurrentMappingContexts() const { return CurrentMappingContexts; }

protected:
	bool bUsingGamepad = false;

	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Atlantis|Controls")
	TObjectPtr<UInputMappingContext> SwimmingMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Atlantis|Controls")
	TObjectPtr<UInputMappingContext> DefaultMovementMappingContext;

	void RefreshMovementControls();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Atlantis|Controls|Ballast")
	TObjectPtr<UInputAction> DescendBallastAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Atlantis|Controls|Ballast")
	TObjectPtr<UInputAction> WanderBallastAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Atlantis|Controls|Ballast")
	TObjectPtr<UInputAction> AscendBallastAction;

	void RequestBallastState(const FInputActionValue& ActionValue, EAtlantisBallastState NewBallastState);

	UFUNCTION(Server, Reliable)
	void ServerSetBallastState(EAtlantisBallastState NewBallastState);

	/** Pushes every context in the desired set to the input subsystem. */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|Controls")
	void EstablishMappingContexts();

	/** Withdraws every context in the desired set from the input subsystem. */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|Controls")
	void ClearMappingContexts();

	/** Null on a remote controller, which is what suppresses input work off the local client. */
	UEnhancedInputLocalPlayerSubsystem* GetEnhancedInputSubsystem() const;

	/** The controls the player should have, whether or not they are currently applied. */
	UPROPERTY(BlueprintReadOnly, Category = "Atlantis|Controls")
	TArray<UInputMappingContext*> CurrentMappingContexts;

	/** Whether CurrentMappingContexts is currently pushed to the input subsystem. */
	UPROPERTY(BlueprintReadOnly, Category = "Atlantis|Controls")
	bool bControlsEnabled = true;
};
