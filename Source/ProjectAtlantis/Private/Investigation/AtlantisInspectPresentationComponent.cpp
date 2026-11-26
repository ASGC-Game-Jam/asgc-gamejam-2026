// Copyright (c) 2026 ASGC

#include "Investigation/AtlantisInspectPresentationComponent.h"
#include "Investigation/AtlantisInspectableComponent.h"
#include "Investigation/AtlantisInspectorComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY_STATIC(LogAtlantisInspectPresentation, Log, All);

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
	BoundInspector = Inspector;
}

void UAtlantisInspectPresentationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UAtlantisInspectorComponent* Inspector = BoundInspector.Get())
	{
		Inspector->OnInspectStarted.RemoveDynamic(this, &UAtlantisInspectPresentationComponent::HandleInspectStarted);
		Inspector->OnInspectEnded.RemoveDynamic(this, &UAtlantisInspectPresentationComponent::HandleInspectEnded);
	}
	BoundInspector.Reset();
	RemoveInspectLight();

	Super::EndPlay(EndPlayReason);
}

void UAtlantisInspectPresentationComponent::HandleInspectStarted(UAtlantisInspectableComponent* Inspectable)
{
	const UAtlantisInspectorComponent* Inspector = BoundInspector.Get();
	if (Inspectable && Inspector)
	{
		AddInspectLight(*Inspectable, Inspector->GetInspectView());
	}
}

void UAtlantisInspectPresentationComponent::HandleInspectEnded(UAtlantisInspectableComponent* Inspectable)
{
	RemoveInspectLight();
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
