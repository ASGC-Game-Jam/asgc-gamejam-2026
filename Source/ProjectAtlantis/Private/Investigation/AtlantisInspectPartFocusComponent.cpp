// Copyright (c) 2026 ASGC

#include "Investigation/AtlantisInspectPartFocusComponent.h"
#include "Investigation/AtlantisInspectableComponent.h"
#include "Investigation/AtlantisInspectorComponent.h"
#include "Investigation/AtlantisInspectPartComponent.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogAtlantisInspectPartFocus, Log, All);

UAtlantisInspectPartFocusComponent::UAtlantisInspectPartFocusComponent()
{
	// Only switched on while inspecting with bDrawPartDebug
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UAtlantisInspectPartFocusComponent::BeginPlay()
{
	Super::BeginPlay();

	UAtlantisInspectorComponent* Inspector = GetOwner()->FindComponentByClass<UAtlantisInspectorComponent>();
	if (!Inspector)
	{
		UE_LOG(LogAtlantisInspectPartFocus, Warning,
			TEXT("%s on %s: no Atlantis Inspector on this actor, so there are no parts to focus."),
			*GetName(), *GetNameSafe(GetOwner()));
		return;
	}

	Inspector->OnInspectStarted.AddDynamic(this, &UAtlantisInspectPartFocusComponent::HandleInspectStarted);
	Inspector->OnInspectEnded.AddDynamic(this, &UAtlantisInspectPartFocusComponent::HandleInspectEnded);
	Inspector->OnInspectRotated.AddDynamic(this, &UAtlantisInspectPartFocusComponent::HandleInspectRotated);
	BoundInspector = Inspector;
}

void UAtlantisInspectPartFocusComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UAtlantisInspectorComponent* Inspector = BoundInspector.Get())
	{
		Inspector->OnInspectStarted.RemoveDynamic(this, &UAtlantisInspectPartFocusComponent::HandleInspectStarted);
		Inspector->OnInspectEnded.RemoveDynamic(this, &UAtlantisInspectPartFocusComponent::HandleInspectEnded);
		Inspector->OnInspectRotated.RemoveDynamic(this, &UAtlantisInspectPartFocusComponent::HandleInspectRotated);
	}
	BoundInspector.Reset();
	ClearParts();

	Super::EndPlay(EndPlayReason);
}

void UAtlantisInspectPartFocusComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	DrawPartDebug();
}

void UAtlantisInspectPartFocusComponent::HandleInspectStarted(UAtlantisInspectableComponent* Inspectable)
{
	ClearParts();
	const AActor* Object = Inspectable ? Inspectable->GetOwner() : nullptr;
	if (!Object)
	{
		return;
	}

	TArray<UAtlantisInspectPartComponent*> FoundParts;
	Object->GetComponents<UAtlantisInspectPartComponent>(FoundParts);
	for (UAtlantisInspectPartComponent* Part : FoundParts)
	{
		Parts.Add(Part);
	}
	if (Parts.IsEmpty())
	{
		return;
	}

	if (PartFocusAngle <= 0.f)
	{
		UE_LOG(LogAtlantisInspectPartFocus, Warning, TEXT("%s on %s: PartFocusAngle not set, the parts of %s can never be focused."),
			*GetName(), *GetNameSafe(GetOwner()), *GetNameSafe(Object));
	}

	UpdatePartFocus();
	SetComponentTickEnabled(ENABLE_DRAW_DEBUG && bDrawPartDebug);
}

void UAtlantisInspectPartFocusComponent::HandleInspectEnded(UAtlantisInspectableComponent* Inspectable)
{
	ClearParts();
}

void UAtlantisInspectPartFocusComponent::ClearParts()
{
	SetComponentTickEnabled(false);
	SetFocusedPart(nullptr);
	Parts.Reset();
}

void UAtlantisInspectPartFocusComponent::SetFocusedPart(UAtlantisInspectPartComponent* NewPart)
{
	UAtlantisInspectPartComponent* OldPart = FocusedPart.Get();
	if (NewPart == OldPart)
	{
		return;
	}

	FocusedPart = NewPart;
	UE_LOG(LogAtlantisInspectPartFocus, Log, TEXT("Focused part: %s"), *GetNameSafe(NewPart));
	OnPartFocusChanged.Broadcast(NewPart, OldPart);
}

bool UAtlantisInspectPartFocusComponent::GetFocusedPartScreenPosition(FVector2D& ScreenPosition) const
{
	const UAtlantisInspectPartComponent* Part = FocusedPart.Get();
	const APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PlayerController = Pawn ? Pawn->GetController<APlayerController>() : nullptr;
	if (!Part || !PlayerController)
	{
		return false;
	}

	// Widget units already account for the DPI scale, unlike raw viewport pixels
	return UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PlayerController, Part->GetComponentLocation(), ScreenPosition, false);
}

void UAtlantisInspectPartFocusComponent::UpdatePartFocus()
{
	const UAtlantisInspectorComponent* Inspector = BoundInspector.Get();
	if (!Inspector || Parts.IsEmpty())
	{
		return;
	}

	const FVector Center = Inspector->GetInspectedObjectCenter();
	const FVector ToCamera = (Inspector->GetInspectView().GetLocation() - Center).GetSafeNormal();
	auto AngleToCamera = [&Center, &ToCamera](const UAtlantisInspectPartComponent& Part)
	{
		const FVector ToPart = (Part.GetComponentLocation() - Center).GetSafeNormal();
		return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(ToPart, ToCamera), -1.f, 1.f)));
	};

	// The focused part keeps its focus until it turns past the angle plus the hysteresis, even if another part lines up better
	UAtlantisInspectPartComponent* Current = FocusedPart.Get();
	if (Current && Current->IsPartEnabled() && AngleToCamera(*Current) <= PartFocusAngle + PartFocusHysteresis)
	{
		return;
	}

	// Otherwise the most aligned enabled part within the angle wins
	UAtlantisInspectPartComponent* Best = nullptr;
	float BestAngle = PartFocusAngle;
	for (const TWeakObjectPtr<UAtlantisInspectPartComponent>& WeakPart : Parts)
	{
		UAtlantisInspectPartComponent* Part = WeakPart.Get();
		if (!Part || !Part->IsPartEnabled())
		{
			continue;
		}

		const float Angle = AngleToCamera(*Part);
		if (Angle <= BestAngle)
		{
			Best = Part;
			BestAngle = Angle;
		}
	}

	SetFocusedPart(Best);
}

void UAtlantisInspectPartFocusComponent::DrawPartDebug() const
{
#if ENABLE_DRAW_DEBUG
	const UAtlantisInspectorComponent* Inspector = BoundInspector.Get();
	if (!Inspector || PartDebugRadius <= 0.f)
	{
		return;
	}

	const FVector Center = Inspector->GetInspectedObjectCenter();
	const UAtlantisInspectPartComponent* Focused = FocusedPart.Get();
	for (const TWeakObjectPtr<UAtlantisInspectPartComponent>& WeakPart : Parts)
	{
		const UAtlantisInspectPartComponent* Part = WeakPart.Get();
		if (!Part)
		{
			continue;
		}

		const FColor Color = Part == Focused ? FColor::Green : (Part->IsPartEnabled() ? FColor::Yellow : FColor::Silver);
		const FVector Location = Part->GetComponentLocation();
		// Foreground so they show through the object they sit on
		DrawDebugSphere(GetWorld(), Location, PartDebugRadius, PartDebugSphereSegments, Color, false, -1.f, SDPG_Foreground);
		DrawDebugLine(GetWorld(), Center, Location, Color, false, -1.f, SDPG_Foreground);
	}
#endif
}
