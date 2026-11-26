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

	/** Rotation when the inspect screen opens, 0 = the +X faces the player */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Framing")
	FRotator StartRotation = FRotator::ZeroRotator;

	/** Added to the Inspector's ScreenOffset for this object only, in the same units, 0 should be fine for most objects */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Framing")
	FVector2D ScreenOffsetNudge = FVector2D::ZeroVector;

	/** Scales the automatic distance for odd shapes above 1 = farther (smaller), below 1 = closer (bigger) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Framing", meta = (ClampMin = "0.01"))
	float DistanceMultiplier = 1.f;

	/** Off for objects the Inspector's light washes out or that already read well (e.g. they glow) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Light")
	bool bUseInspectLight = true;

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
