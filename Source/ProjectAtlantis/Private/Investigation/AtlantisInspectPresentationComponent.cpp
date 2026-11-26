// Copyright (c) 2026 ASGC

#include "Investigation/AtlantisInspectPresentationComponent.h"
#include "Investigation/AtlantisInspectableComponent.h"
#include "Investigation/AtlantisInspectorComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY_STATIC(LogAtlantisInspectPresentation, Log, All);

namespace AtlantisInspectDimParams
{
	// Scalar parameter names inside MPP_InspectDim
	const FName FocusDistance(TEXT("FocusDistance"));
	const FName FocusRadius(TEXT("FocusRadius"));
	const FName DimFalloff(TEXT("DimFalloff"));
	const FName DimAmount(TEXT("DimAmount"));
}

UAtlantisInspectPresentationComponent::UAtlantisInspectPresentationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAtlantisInspectPresentationComponent::BeginPlay()
{
	Super::BeginPlay();

	UAtlantisInspectorComponent* Inspector = GetOwner()->FindComponentByClass<UAtlantisInspectorComponent>();
	if (!Inspector)
	{
		UE_LOG(LogAtlantisInspectPresentation, Warning,
			TEXT("%s on %s: no Atlantis Inspector on this actor, so there is nothing to present."),
			*GetName(), *GetNameSafe(GetOwner()));
		return;
	}

	Inspector->OnInspectStarted.AddDynamic(this, &UAtlantisInspectPresentationComponent::HandleInspectStarted);
	Inspector->OnInspectEnded.AddDynamic(this, &UAtlantisInspectPresentationComponent::HandleInspectEnded);
	Inspector->OnInspectZoomChanged.AddDynamic(this, &UAtlantisInspectPresentationComponent::HandleInspectZoomChanged);
	BoundInspector = Inspector;
}

void UAtlantisInspectPresentationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UAtlantisInspectorComponent* Inspector = BoundInspector.Get())
	{
		Inspector->OnInspectStarted.RemoveDynamic(this, &UAtlantisInspectPresentationComponent::HandleInspectStarted);
		Inspector->OnInspectEnded.RemoveDynamic(this, &UAtlantisInspectPresentationComponent::HandleInspectEnded);
		Inspector->OnInspectZoomChanged.RemoveDynamic(this, &UAtlantisInspectPresentationComponent::HandleInspectZoomChanged);
	}
	BoundInspector.Reset();
	RemoveInspectLight();
	RemoveBackgroundEffects();

	Super::EndPlay(EndPlayReason);
}

void UAtlantisInspectPresentationComponent::HandleInspectStarted(UAtlantisInspectableComponent* Inspectable)
{
	const UAtlantisInspectorComponent* Inspector = BoundInspector.Get();
	if (Inspectable && Inspector)
	{
		AddInspectLight(*Inspectable, Inspector->GetInspectView());
		AddBackgroundEffects();
	}
}

void UAtlantisInspectPresentationComponent::HandleInspectEnded(UAtlantisInspectableComponent* Inspectable)
{
	RemoveInspectLight();
	RemoveBackgroundEffects();
}

void UAtlantisInspectPresentationComponent::AddInspectLight(const UAtlantisInspectableComponent& Inspectable, const FTransform& View)
{
	if (!bEnableInspectLight || !Inspectable.bUseInspectLight)
	{
		return;
	}

	if (LightIntensity <= 0.f || LightAttenuationRadius <= 0.f)
	{
		UE_LOG(LogAtlantisInspectPresentation, Warning,
			TEXT("%s on %s: LightIntensity or LightAttenuationRadius not set, inspecting without the light."),
			*GetName(), *GetNameSafe(GetOwner()));
		return;
	}
	// We set up the light
	InspectLight = NewObject<UPointLightComponent>(GetOwner(), NAME_None, RF_Transient);
	InspectLight->SetMobility(EComponentMobility::Movable);
	InspectLight->SetIntensityUnits(ELightUnits::Candelas);
	InspectLight->SetIntensity(LightIntensity);
	InspectLight->SetLightColor(LightColor);
	InspectLight->SetAttenuationRadius(LightAttenuationRadius);
	InspectLight->SetCastShadows(bLightCastsShadows);
	InspectLight->SetWorldLocation(View.TransformPosition(LightOffset));
	InspectLight->RegisterComponent();
}

void UAtlantisInspectPresentationComponent::RemoveInspectLight()
{
	if (InspectLight)
	{
		InspectLight->DestroyComponent();
		InspectLight = nullptr;
	}
}

void UAtlantisInspectPresentationComponent::AddBackgroundEffects()
{
	const bool bBlur = bEnableBackgroundBlur && BlurFstop > 0.f;
	const bool bDim = bEnableBackgroundDim && DimMaterial && DimFalloff > 0.f;
	if (bEnableBackgroundBlur && !bBlur)
	{
		UE_LOG(LogAtlantisInspectPresentation, Warning, TEXT("%s on %s: BlurFstop not set, inspecting without the blur."),
			*GetName(), *GetNameSafe(GetOwner()));
	}
	if (bEnableBackgroundDim && !bDim)
	{
		UE_LOG(LogAtlantisInspectPresentation, Warning, TEXT("%s on %s: DimMaterial or DimFalloff not set, inspecting without the dim."),
			*GetName(), *GetNameSafe(GetOwner()));
	}
	if (!bBlur && !bDim)
	{
		return;
	}

	BackgroundPostProcess = NewObject<UPostProcessComponent>(GetOwner(), NAME_None, RF_Transient);
	BackgroundPostProcess->bUnbound = true;
	BackgroundPostProcess->Priority = BackgroundPostProcessPriority;
	FPostProcessSettings& Settings = BackgroundPostProcess->Settings;

	if (bBlur)
	{
		Settings.bOverride_DepthOfFieldFocalDistance = true;
		Settings.bOverride_DepthOfFieldFstop = true;
		Settings.DepthOfFieldFstop = BlurFstop;
		//low values would be ignored
		Settings.bOverride_DepthOfFieldMinFstop = true;
		Settings.DepthOfFieldMinFstop = BlurFstop;
	}

	if (bDim)
	{
		DimMaterialInstance = UMaterialInstanceDynamic::Create(DimMaterial, this);
		DimMaterialInstance->SetScalarParameterValue(AtlantisInspectDimParams::DimAmount, DimAmount);
		DimMaterialInstance->SetScalarParameterValue(AtlantisInspectDimParams::DimFalloff, DimFalloff);
		Settings.WeightedBlendables.Array.Add(FWeightedBlendable(1.f, DimMaterialInstance));
	}

	BackgroundPostProcess->RegisterComponent();
	UpdateBackgroundFocus();
}

void UAtlantisInspectPresentationComponent::RemoveBackgroundEffects()
{
	if (BackgroundPostProcess)
	{
		BackgroundPostProcess->DestroyComponent();
		BackgroundPostProcess = nullptr;
	}
	DimMaterialInstance = nullptr;
}

void UAtlantisInspectPresentationComponent::UpdateBackgroundFocus()
{
	const UAtlantisInspectorComponent* Inspector = BoundInspector.Get();
	if (!BackgroundPostProcess || !Inspector)
	{
		return;
	}

	// Both measure depth along the view direction, the same way GetInspectedObjectDistance does
	const float Distance = Inspector->GetInspectedObjectDistance();
	BackgroundPostProcess->Settings.DepthOfFieldFocalDistance = Distance;
	if (DimMaterialInstance)
	{
		DimMaterialInstance->SetScalarParameterValue(AtlantisInspectDimParams::FocusDistance, Distance);
		DimMaterialInstance->SetScalarParameterValue(AtlantisInspectDimParams::FocusRadius, Inspector->GetInspectedObjectRadius());
	}
}
