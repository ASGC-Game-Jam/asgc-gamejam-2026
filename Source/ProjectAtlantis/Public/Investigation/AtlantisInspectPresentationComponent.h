// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/SlateWrapperTypes.h"
#include "AtlantisInspectPresentationComponent.generated.h"

class UAtlantisInspectableComponent;
class UAtlantisInspectorComponent;
class UPointLightComponent;
class UPostProcessComponent;
class UPrimitiveComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UAtlantisInspectWidget;
class UUserWidget;


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

	/** Blurs everything in front of and behind the object */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Background")
	bool bEnableBackgroundBlur = false;

	/** Camera aperture for the blur: lower = blurrier background */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Background",
		meta = (ClampMin = "0.01", EditCondition = "bEnableBackgroundBlur"))
	float BlurFstop = 0.f;

	/** Darkens everything except the object */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Background")
	bool bEnableBackgroundDim = false;

	/** Post process material that does the dimming */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Background",
		meta = (EditCondition = "bEnableBackgroundDim"))
	TObjectPtr<UMaterialInterface> DimMaterial;

	/** How dark the background gets*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Background",
		meta = (ClampMin = "0", ClampMax = "1", EditCondition = "bEnableBackgroundDim"))
	float DimAmount = 0.f;

	/** Widget shown while inspecting */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Widget")
	TSubclassOf<UAtlantisInspectWidget> InspectWidgetClass;

	/** Hides every other widget on screen (HUD, hints...) while inspecting so nothing overlaps, and restores them after */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Widget")
	bool bHideOtherWidgets = false;

	/** Widgets that stay on screen even when Hide Other Widgets is on (children of these classes too) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Widget",
		meta = (EditCondition = "bHideOtherWidgets"))
	TArray<TSubclassOf<UUserWidget>> WidgetsKeptVisible;

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
	void AddBackgroundEffects(const UAtlantisInspectableComponent& Inspectable);

	/** Destroys what AddBackgroundEffects created */
	void RemoveBackgroundEffects();

	/** Points the blur at the object's current distance. Called on start and whenever the zoom changes */
	void UpdateBackgroundFocus();

	/** Makes the object write Custom Depth, which is how the dim material tells the object apart from the background */
	void MaskObjectFromDim(const AActor& Object);

	/** Turns Custom Depth back off on what MaskObjectFromDim turned on */
	void UnmaskObjectFromDim();

	/** Creates InspectWidgetClass on the player's screen and hands it the object */
	void ShowInspectWidget(UAtlantisInspectorComponent& Inspector, UAtlantisInspectableComponent& Inspectable);

	/** Tells the widget it's ending, then removes it */
	void HideInspectWidget(UAtlantisInspectableComponent* Inspectable);

	/** Collapses every visible widget on screen except the ones in WidgetsKeptVisible */
	void HideOtherWidgets();

	/** Gives the widgets HideOtherWidgets collapsed their old visibility back */
	void RestoreOtherWidgets();

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

	/** Object distance when inspecting started (zoom 1). BlurFstop is the look at this distance */
	float BlurBaseDistance = 0.f;

	/** DimMaterial's runtime copy, carrying DimAmount */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DimMaterialInstance;

	/** The object's parts MaskObjectFromDim turned Custom Depth on for (parts that already had it are left alone) */
	TArray<TWeakObjectPtr<UPrimitiveComponent>> DimMaskedPrimitives;

	UPROPERTY(Transient)
	TObjectPtr<UAtlantisInspectWidget> InspectWidget;

	/** Widgets HideOtherWidgets collapsed, with the visibility each had. Weak so a widget removed mid-inspect is just skipped */
	TMap<TWeakObjectPtr<UUserWidget>, ESlateVisibility> HiddenWidgets;
};
