// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SmartObjectTypes.h"
#include "SmartObjectRuntime.h"
#include "AtlantisHidingComponent.generated.h"

class UInputMappingContext;

/**
 * Player HidingState changed. True while the player is hiding.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHidingStateChanged, bool, bHiding);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTATLANTIS_API UAtlantisHidingComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	
	UAtlantisHidingComponent();

	/** Tries to enter in a hiding spot. Returns outcome. (true = successfully entered hiding spot). */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|Stealth")
	bool EnterHidingSpot(AActor* HidingSpot);

	/** Leaves the current hiding spot and releases it. Returns outcome. (true = successfully left hiding spot). */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|Stealth")
	bool ExitHidingSpot();

	/** Indicates whether the owner is currently inside a hiding spot. */
	UFUNCTION(BlueprintPure, Category = "Atlantis|Stealth")
	bool IsHiding() const { return ClaimHandle.IsValid(); }

	/** Communicates when the owner hides\unhides. */
	UPROPERTY(BlueprintAssignable, Category = "Atlantis|Stealth")
	FOnHidingStateChanged OnHidingStateChanged;

private:
	/** Hides the owner in game, moves it in the hiding spot, and disables  inputs. Returns outcome. */
	bool ApplyHiding(const FTransform& SlotTransform);

	/** Restores the owner to its state before hiding. Returns outcome. */
	bool RestoreFromHiding();

	/** Claim on the occupied slot. IsValid returns true only while hiding. */
	FSmartObjectClaimHandle ClaimHandle;

	/** Owner transform before entering the hiding spot, restored on exit. */
	FTransform PreviousTransform;

	/** Inputs restricted while hiding */
	UPROPERTY(EditDefaultsOnly, Category = "Atlantis|Stealth")
	TArray<TObjectPtr<UInputMappingContext>> RestrictedMappingContexts;

};
