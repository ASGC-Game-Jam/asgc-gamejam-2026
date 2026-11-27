// Copyright (c) 2026 ASGC

#include "Investigation/AtlantisInspectPresentationComponent.h"
#include "Investigation/AtlantisInspectableComponent.h"
#include "Investigation/AtlantisInspectorComponent.h"
#include "UI/AtlantisInspectWidget.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogAtlantisInspectPresentation, Log, All);

namespace AtlantisInspectDimParams
{
	// Scalar parameter name inside MPP_InspectDim
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
	HideInspectWidget(nullptr);
	RestoreOtherWidgets();

	Super::EndPlay(EndPlayReason);
}

void UAtlantisInspectPresentationComponent::HandleInspectStarted(UAtlantisInspectableComponent* Inspectable)
{
	UAtlantisInspectorComponent* Inspector = BoundInspector.Get();
	if (Inspectable && Inspector)
	{
		AddInspectLight(*Inspectable, Inspector->GetInspectView());
		AddBackgroundEffects(*Inspectable);
		// Before showing ours, so the inspect widget is never caught by it
		HideOtherWidgets();
		ShowInspectWidget(*Inspector, *Inspectable);
	}
}

void UAtlantisInspectPresentationComponent::HandleInspectEnded(UAtlantisInspectableComponent* Inspectable)
{
	HideInspectWidget(Inspectable);
	RestoreOtherWidgets();
	RemoveInspectLight();
	RemoveBackgroundEffects();
}

void UAtlantisInspectPresentationComponent::ShowInspectWidget(UAtlantisInspectorComponent& Inspector, UAtlantisInspectableComponent& Inspectable)
{
	if (!InspectWidgetClass)
	{
		return;
	}

	const APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PlayerController = Pawn ? Pawn->GetController<APlayerController>() : nullptr;
	if (!PlayerController)
	{
		return;
	}

	InspectWidget = CreateWidget<UAtlantisInspectWidget>(PlayerController, InspectWidgetClass);
	if (!InspectWidget)
	{
		return;
	}

	InspectWidget->AddToPlayerScreen();
	InspectWidget->StartInspect(&Inspector, &Inspectable);
}

void UAtlantisInspectPresentationComponent::HideInspectWidget(UAtlantisInspectableComponent* Inspectable)
{
	if (InspectWidget)
	{
		InspectWidget->EndInspect(Inspectable);
		InspectWidget->RemoveFromParent();
		InspectWidget = nullptr;
	}
}

void UAtlantisInspectPresentationComponent::HideOtherWidgets()
{
	if (!bHideOtherWidgets)
	{
		return;
	}

	// Top level only: hiding a parent (like the HUD root) already hides everything inside it
	TArray<UUserWidget*> OnScreenWidgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, OnScreenWidgets, UUserWidget::StaticClass(), true);

	for (UUserWidget* Widget : OnScreenWidgets)
	{
		if (!Widget || Widget == InspectWidget || !Widget->IsVisible())
		{
			continue;
		}

		const bool bKept = WidgetsKeptVisible.ContainsByPredicate([Widget](const TSubclassOf<UUserWidget>& KeptClass)
		{
			return KeptClass && Widget->IsA(KeptClass);
		});
		if (bKept)
		{
			continue;
		}

		HiddenWidgets.Add(Widget, Widget->GetVisibility());
		Widget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UAtlantisInspectPresentationComponent::RestoreOtherWidgets()
{
	for (const TPair<TWeakObjectPtr<UUserWidget>, ESlateVisibility>& Hidden : HiddenWidgets)
	{
		if (UUserWidget* Widget = Hidden.Key.Get())
		{
			Widget->SetVisibility(Hidden.Value);
		}
	}
	HiddenWidgets.Reset();
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

void UAtlantisInspectPresentationComponent::AddBackgroundEffects(const UAtlantisInspectableComponent& Inspectable)
{
	const AActor* Object = Inspectable.GetOwner();
	const bool bBlur = bEnableBackgroundBlur && BlurFstop > 0.f;
	const bool bDim = bEnableBackgroundDim && DimMaterial && Object;
	if (bEnableBackgroundBlur && !bBlur)
	{
		UE_LOG(LogAtlantisInspectPresentation, Warning, TEXT("%s on %s: BlurFstop not set, inspecting without the blur."),
			*GetName(), *GetNameSafe(GetOwner()));
	}
	if (bEnableBackgroundDim && !bDim)
	{
		UE_LOG(LogAtlantisInspectPresentation, Warning, TEXT("%s on %s: DimMaterial not set, inspecting without the dim."),
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
		// The fstop itself is set by UpdateBackgroundFocus, it depends on the zoom
		Settings.bOverride_DepthOfFieldFocalDistance = true;
		Settings.bOverride_DepthOfFieldFstop = true;
		//low values would be ignored
		Settings.bOverride_DepthOfFieldMinFstop = true;
		if (const UAtlantisInspectorComponent* Inspector = BoundInspector.Get())
		{
			BlurBaseDistance = Inspector->GetInspectedObjectDistance();
		}
	}

	if (bDim)
	{
		DimMaterialInstance = UMaterialInstanceDynamic::Create(DimMaterial, this);
		DimMaterialInstance->SetScalarParameterValue(AtlantisInspectDimParams::DimAmount, DimAmount);
		Settings.WeightedBlendables.Array.Add(FWeightedBlendable(1.f, DimMaterialInstance));
		MaskObjectFromDim(*Object);
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
	BlurBaseDistance = 0.f;
	UnmaskObjectFromDim();
}

void UAtlantisInspectPresentationComponent::MaskObjectFromDim(const AActor& Object)
{
	TArray<UPrimitiveComponent*> Primitives;
	Object.GetComponents<UPrimitiveComponent>(Primitives);
	for (UPrimitiveComponent* Primitive : Primitives)
	{
		if (Primitive && Primitive->IsVisible() && !Primitive->bRenderCustomDepth)
		{
			Primitive->SetRenderCustomDepth(true);
			DimMaskedPrimitives.Add(Primitive);
		}
	}
}

void UAtlantisInspectPresentationComponent::UnmaskObjectFromDim()
{
	for (const TWeakObjectPtr<UPrimitiveComponent>& Primitive : DimMaskedPrimitives)
	{
		if (UPrimitiveComponent* Masked = Primitive.Get())
		{
			Masked->SetRenderCustomDepth(false);
		}
	}
	DimMaskedPrimitives.Reset();
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
	FPostProcessSettings& Settings = BackgroundPostProcess->Settings;
	Settings.DepthOfFieldFocalDistance = Distance;
	if (Settings.bOverride_DepthOfFieldFstop && BlurBaseDistance > 0.f && Distance > 0.f)
	{
		// Background blur grows as the focus gets closer (~ 1 / (Fstop * Distance)), which makes the background
		// look like it moves while zooming. Scaling the fstop the other way keeps it the same at every zoom
		const float Fstop = BlurFstop * BlurBaseDistance / Distance;
		Settings.DepthOfFieldFstop = Fstop;
		Settings.DepthOfFieldMinFstop = Fstop;
	}
}
