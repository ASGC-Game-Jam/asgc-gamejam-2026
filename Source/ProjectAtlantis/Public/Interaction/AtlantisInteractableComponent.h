// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AtlantisInteractableComponent.generated.h"

class UMaterialInterface;
class UMeshComponent;

/** Fired on the interactable when an interactor (the player) uses it */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAtlantisOnInteracted, AActor*, Interactor);

/** Fired when an interactor starts or stops looking at this interactable*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAtlantisOnInteractableFocusChanged, AActor*, Interactor);


/**
 * Not spawnable directly: designers add a Blueprint child, which holds all configuration
 * (prompt text, highlight material...). Nothing is hard-coded here.
 */
UCLASS(Blueprintable, ClassGroup=(Atlantis), meta=(DisplayName="Atlantis Interactable"))
class PROJECTATLANTIS_API UAtlantisInteractableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAtlantisInteractableComponent();

	/** Text a prompt widget can show for this interactable like "Talk" */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlantis|Interaction")
	FText PromptText;

	/** If true, the interactable disables itself after its first successful interaction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlantis|Interaction")
	bool bSingleUse = false;

	/** If true, the owner's meshes get HighlightMaterial as overlay while the player is looking at it */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlantis|Interaction|Highlight")
	bool bHighlightWhenFocused = true;

	/** Overlay material applied to the owner's meshes while focused */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlantis|Interaction|Highlight")
	TObjectPtr<UMaterialInterface> HighlightMaterial;

	/** Called when an interactor uses this interactable. Put the object-specific behavior here */
	UPROPERTY(BlueprintAssignable, Category = "Atlantis|Interaction")
	FAtlantisOnInteracted OnInteracted;

	/** Called when an interactor starts looking at this interactable */
	UPROPERTY(BlueprintAssignable, Category = "Atlantis|Interaction")
	FAtlantisOnInteractableFocusChanged OnFocused;

	/** Called when an interactor stops looking at this interactable */
	UPROPERTY(BlueprintAssignable, Category = "Atlantis|Interaction")
	FAtlantisOnInteractableFocusChanged OnUnfocused;

	/** Runs the interaction if allowed. Returns true if OnInteracted was broadcast */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|Interaction")
	bool TryInteract(AActor* Interactor);

	/** Whether this interactable can currently be used */
	UFUNCTION(BlueprintPure, Category = "Atlantis|Interaction")
	bool CanInteract() const { return bEnabled; }

	/** Turns the interactable on/off */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|Interaction")
	void SetInteractionEnabled(bool bNewEnabled);

	/** Whether an interactor is currently looking at this interactable. */
	UFUNCTION(BlueprintPure, Category = "Atlantis|Interaction")
	bool IsFocused() const;

	/** Whether this specific interactor is currently looking at this interactable */
	UFUNCTION(BlueprintPure, Category = "Atlantis|Interaction")
	bool IsFocusedBy(AActor* Interactor) const { return Interactor && FocusingInteractors.Contains(Interactor); }

protected:
	/** The interactor is the only class outside this one (and subclasses) allowed to call the Notify functions */
	friend class UAtlantisInteractorComponent;

	/** Called by UAtlantisInteractorComponent. Not exposed to Blueprints */
	void NotifyFocused(AActor* Interactor);

	/** Called by UAtlantisInteractorComponent. Not exposed to Blueprints */
	void NotifyUnfocused(AActor* Interactor);

	/** Starting enabled state. Use SetInteractionEnabled at runtime */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Interaction", meta = (DisplayName = "Enabled"))
	bool bEnabled = true;

	/** Warns if the Blueprint is missing configuration */
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Applies or removes HighlightMaterial on the owner's meshes, restoring their previous overlay */
	void SetHighlighted(bool bHighlighted);

	/** Everyone currently looking at this. The highlight stays on until the last one looks away */
	TSet<TWeakObjectPtr<AActor>> FocusingInteractors;

	/** Overlay materials the meshes had before highlighting, so they can be restored */
	TMap<TWeakObjectPtr<UMeshComponent>, TWeakObjectPtr<UMaterialInterface>> SavedOverlayMaterials;
};
