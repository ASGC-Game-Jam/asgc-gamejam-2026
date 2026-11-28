// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "AtlantisInteractorComponent.generated.h"

class UAtlantisInteractableComponent;
class UInputAction;
class UInputComponent;

/** Fired when the interactable the player is looking at changes */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAtlantisOnInteractFocusChanged,
	UAtlantisInteractableComponent*, NewFocus, UAtlantisInteractableComponent*, OldFocus);


UCLASS(Blueprintable, ClassGroup=(Atlantis), meta=(DisplayName="Atlantis Interactor"))
class PROJECTATLANTIS_API UAtlantisInteractorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAtlantisInteractorComponent();

	/** Max distance from the owner (not the camera) at which things can be interacted with. This should be set on Blueprint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlantis|Interaction", meta = (ClampMin = "0"))
	float InteractionRange = 0.f;

	/** Radius of the focus sweep. Bigger is more forgiving when aiming at small objects. 0 = line trace. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlantis|Interaction", meta = (ClampMin = "0"))
	float TraceRadius = 0.f;

	/** Channel the focus sweep uses. Set it to the project's "Interact" trace channel in the Blueprint */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlantis|Interaction")
	TEnumAsByte<ECollisionChannel> TraceChannel;

	/** Input action that triggers Interact(). Bound automatically on the owner's input component */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Interaction")
	TObjectPtr<UInputAction> InteractAction;

	/** Draws the focus sweep */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlantis|Interaction|Debug")
	bool bDrawDebug = false;

	/** Fired when the focused interactable changes */
	UPROPERTY(BlueprintAssignable, Category = "Atlantis|Interaction")
	FAtlantisOnInteractFocusChanged OnFocusChanged;

	/** Interacts with the focused interactable. Returns true if an interaction happened */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|Interaction")
	bool Interact();

	/** The interactable currently being looked at, or null. */
	UFUNCTION(BlueprintPure, Category = "Atlantis|Interaction")
	UAtlantisInteractableComponent* GetFocusedInteractable() const { return FocusedInteractable.Get(); }

	/** Re-runs the focus trace immediately. Runs every tick. */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|Interaction")
	void RefreshFocus();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	/** Warns if the Blueprint is missing configuration */
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Finds the interactable in front of the owner, or null */
	UAtlantisInteractableComponent* FindInteractable() const;

	/** View location/direction: the player's camera if possessed, otherwise the owner's eyes */
	void GetViewPoint(FVector& OutLocation, FRotator& OutRotation) const;

	void SetFocus(UAtlantisInteractableComponent* NewFocus);

	/** True if the owner is a pawn controlled by a player, so focus should be shown (highlight, OnFocused) */
	bool ShouldShowFocus() const;

	/** Binds InteractAction on the owner's input component. Retried each tick because input is set up after possession */
	void TryBindInput();

	void HandleInteractInput() { Interact(); }

	TWeakObjectPtr<UAtlantisInteractableComponent> FocusedInteractable;

	/** The interactable this component told it is being looked at (only ever set for player-controlled owners) */
	TWeakObjectPtr<UAtlantisInteractableComponent> ShownFocus;

	TWeakObjectPtr<UInputComponent> BoundInputComponent;
};
