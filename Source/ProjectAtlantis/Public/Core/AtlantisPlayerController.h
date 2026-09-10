// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AtlantisPlayerController.generated.h"

class UInputMappingContext;
class UEnhancedInputLocalPlayerSubsystem;

/** The desired set of control mapping contexts changed. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnControlsChanged);

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
UCLASS()
class PROJECTATLANTIS_API AAtlantisPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
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

	UFUNCTION(BlueprintPure, Category = "Atlantis|Controls")
	const TArray<UInputMappingContext*> GetCurrentMappingContexts() const { return CurrentMappingContexts; }

protected:
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
