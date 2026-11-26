// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AtlantisInspectPresentationComponent.generated.h"

class UAtlantisInspectableComponent;
class UAtlantisInspectorComponent;
class UPointLightComponent;


UCLASS(Blueprintable, ClassGroup=(Atlantis), meta=(DisplayName="Atlantis Inspect Presentation"))
class PROJECTATLANTIS_API UAtlantisInspectPresentationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAtlantisInspectPresentationComponent();

	/** Adds a point light near the camera while inspecting, Objects can opt out */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Light")
	bool bEnableInspectLight = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Light",
		meta = (ClampMin = "0", EditCondition = "bEnableInspectLight"))
	float LightIntensity = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Light",
		meta = (EditCondition = "bEnableInspectLight"))
	FLinearColor LightColor = FLinearColor::White;

	/** How far the light reaches */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Light",
		meta = (ClampMin = "0", EditCondition = "bEnableInspectLight"))
	float LightAttenuationRadius = 0.f;

	/** Where the light sits relative to the camera */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Light",
		meta = (EditCondition = "bEnableInspectLight"))
	FVector LightOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Light",
		meta = (EditCondition = "bEnableInspectLight"))
	bool bLightCastsShadows = false;

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleInspectStarted(UAtlantisInspectableComponent* Inspectable);

	UFUNCTION()
	void HandleInspectEnded(UAtlantisInspectableComponent* Inspectable);

	/** Creates the inspect light at LightOffset from the inspect view */
	void AddInspectLight(const UAtlantisInspectableComponent& Inspectable, const FTransform& View);

	/** Destroys the light AddInspectLight created */
	void RemoveInspectLight();

	/** The Inspector we listen to, so we can unbind on EndPlay */
	TWeakObjectPtr<UAtlantisInspectorComponent> BoundInspector;

	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> InspectLight;
};
