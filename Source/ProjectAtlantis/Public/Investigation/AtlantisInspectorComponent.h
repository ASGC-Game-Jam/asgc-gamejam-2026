// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AtlantisInspectorComponent.generated.h"

class UAtlantisInspectableComponent;

UCLASS(Blueprintable, ClassGroup=(Atlantis), meta=(DisplayName="Atlantis Inspector"))
class PROJECTATLANTIS_API UAtlantisInspectorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAtlantisInspectorComponent();

	/** Opens the inspect screen on this object, returns false on an invalid inspection */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|Inspection")
	bool BeginInspect(UAtlantisInspectableComponent* Inspectable);

	/** Closes the inspect screen*/
	UFUNCTION(BlueprintCallable, Category = "Atlantis|Inspection")
	void EndInspect();

	UFUNCTION(BlueprintPure, Category = "Atlantis|Inspection")
	bool IsInspecting() const { return InspectedObject.IsValid(); }

	/** The object being inspected, or null */
	UFUNCTION(BlueprintPure, Category = "Atlantis|Inspection")
	UAtlantisInspectableComponent* GetInspectedObject() const { return InspectedObject.Get(); }

private:
	TWeakObjectPtr<UAtlantisInspectableComponent> InspectedObject;
};
