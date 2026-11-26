// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AtlantisInspectPresentationComponent.generated.h"

class UAtlantisInspectableComponent;
class UAtlantisInspectorComponent;
class UPointLightComponent;
class UPostProcessComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;


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

	/** Blurs everything in front of and behind the object (depth of field focused on it) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Background")
	bool bEnableBackgroundBlur = false;

	/** Camera aperture for the blur: lower = blurrier background, but too low also blurs the object's own front and back */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Background",
		meta = (ClampMin = "0.01", EditCondition = "bEnableBackgroundBlur"))
	float BlurFstop = 0.f;

	/** Darkens everything except the object */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Background")
	bool bEnableBackgroundDim = false;

	/** Post process material that does the dimming (MPP_InspectDim) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Background",
		meta = (EditCondition = "bEnableBackgroundDim"))
	TObjectPtr<UMaterialInterface> DimMaterial;

	/** How dark the background gets: 0 = untouched, 1 = black */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Background",
		meta = (ClampMin = "0", ClampMax = "1", EditCondition = "bEnableBackgroundDim"))
	float DimAmount = 0.f;

	/** Distance (cm) past the object over which the dim fades in, so it doesn't start with a hard line */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Background",
		meta = (ClampMin = "1", EditCondition = "bEnableBackgroundDim"))
	float DimFalloff = 0.f;

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleInspectStarted(UAtlantisInspectableComponent* Inspectable);

	UFUNCTION()
	void HandleInspectEnded(UAtlantisInspectableComponent* Inspectable);

	UFUNCTION()
	void HandleInspectZoomChanged() { UpdateBackgroundFocus(); }

	/** Creates the inspect light at LightOffset from the inspect view */
	void AddInspectLight(const UAtlantisInspectableComponent& Inspectable, const FTransform& View);

	/** Destroys the light AddInspectLight created */
	void RemoveInspectLight();

	/** Creates the post process that blurs and/or dims the background, whichever is enabled */
	void AddBackgroundEffects();

	/** Destroys what AddBackgroundEffects created */
	void RemoveBackgroundEffects();

	/** Points the blur and the dim at the object's current distance. Called on start and whenever the zoom changes */
	void UpdateBackgroundFocus();

	/**
	 * Wins over level post process volumes (they default to 0) so the blur and dim always show.
	 * Constant on purpose, like the Inspector's input priority
	 */
	static constexpr float BackgroundPostProcessPriority = 1000.f;

	/** The Inspector we listen to, so we can unbind on EndPlay */
	TWeakObjectPtr<UAtlantisInspectorComponent> BoundInspector;

	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> InspectLight;

	UPROPERTY(Transient)
	TObjectPtr<UPostProcessComponent> BackgroundPostProcess;

	/** DimMaterial's runtime copy, so we can set its focus every zoom */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DimMaterialInstance;
};
