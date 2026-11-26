// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AtlantisInspectableComponent.generated.h"

class UAtlantisInteractableComponent;

UCLASS(Blueprintable, ClassGroup=(Atlantis), meta=(DisplayName="Atlantis Inspectable"))
class PROJECTATLANTIS_API UAtlantisInspectableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAtlantisInspectableComponent();

	/** Name shown on the inspect screen */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection")
	FText DisplayName;

	/** Flavor text shown on the inspect screen */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection", meta = (MultiLine = "true"))
	FText Description;

protected:
	/** Hooks into the sibling interactable so interacting opens the inspect screen */
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Bound to the sibling interactable's OnInteracted */
	UFUNCTION()
	void HandleInteracted(AActor* Interactor);

	/** The interactable we listen to, so we can unbind on EndPlay */
	TWeakObjectPtr<UAtlantisInteractableComponent> BoundInteractable;
};
