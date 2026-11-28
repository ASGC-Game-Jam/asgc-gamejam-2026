// Copyright (c) 2026 ASGC

#include "Interaction/AtlantisInteractorComponent.h"
#include "Interaction/AtlantisInteractableComponent.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "EnhancedInputComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "InputAction.h"

DEFINE_LOG_CATEGORY_STATIC(LogAtlantisInteractor, Log, All);

namespace InteractorTraceSettings
{

	constexpr bool bTraceComplex = false;

	constexpr bool bDebugPersistentLines = false;
	constexpr float DebugLifeTime = 0.f;
	constexpr uint8 DebugDepthPriority = 0;
	constexpr float DebugThickness = 1.f;
}

UAtlantisInteractorComponent::UAtlantisInteractorComponent()
{
	// Ticks to keep the focus trace up to date.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UAtlantisInteractorComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!InteractAction)
	{
		UE_LOG(LogAtlantisInteractor, Warning, TEXT("%s on %s: InteractAction is not set, so the interact button does nothing."),
			*GetName(), *GetNameSafe(GetOwner()));
	}
	if (InteractionRange <= 0.f)
	{
		UE_LOG(LogAtlantisInteractor, Warning, TEXT("%s on %s: InteractionRange is 0, so nothing can be focused."),
			*GetName(), *GetNameSafe(GetOwner()));
	}
}

void UAtlantisInteractorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	TryBindInput();
	RefreshFocus();
}

void UAtlantisInteractorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetFocus(nullptr);
	Super::EndPlay(EndPlayReason);
}

bool UAtlantisInteractorComponent::Interact()
{
	RefreshFocus();
	UAtlantisInteractableComponent* Target = FocusedInteractable.Get();
	return Target && Target->TryInteract(GetOwner());
}

void UAtlantisInteractorComponent::RefreshFocus()
{
	SetFocus(FindInteractable());
}

UAtlantisInteractableComponent* UAtlantisInteractorComponent::FindInteractable() const
{
	const AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return nullptr;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	GetViewPoint(ViewLocation, ViewRotation);
	const FVector Direction = ViewRotation.Vector();
	
	const float OwnerDistanceAlongView = FMath::Max(0.f, FVector::DotProduct(Owner->GetActorLocation() - ViewLocation, Direction));
	const FVector Start = ViewLocation + Direction * OwnerDistanceAlongView;
	const FVector End = Start + Direction * InteractionRange;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(AtlantisInteractorTrace), InteractorTraceSettings::bTraceComplex, Owner);
	FHitResult Hit;
	const bool bHit = World->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, TraceChannel,
		FCollisionShape::MakeSphere(TraceRadius), Params);

#if ENABLE_DRAW_DEBUG
	if (bDrawDebug)
	{
		DrawDebugLine(World, Start, bHit ? Hit.Location : End, bHit ? FColor::Green : FColor::Red,
			InteractorTraceSettings::bDebugPersistentLines, InteractorTraceSettings::DebugLifeTime,
			InteractorTraceSettings::DebugDepthPriority, InteractorTraceSettings::DebugThickness);
	}
#endif

	if (!bHit || !Hit.GetActor())
	{
		return nullptr;
	}

	TArray<UAtlantisInteractableComponent*> Interactables;
	Hit.GetActor()->GetComponents<UAtlantisInteractableComponent>(Interactables);
	for (UAtlantisInteractableComponent* Interactable : Interactables)
	{
		if (Interactable->CanInteract())
		{
			return Interactable;
		}
	}
	return nullptr;
}

void UAtlantisInteractorComponent::GetViewPoint(FVector& OutLocation, FRotator& OutRotation) const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (const AController* Controller = Pawn ? Pawn->GetController() : nullptr)
	{
		Controller->GetPlayerViewPoint(OutLocation, OutRotation);
		return;
	}
	GetOwner()->GetActorEyesViewPoint(OutLocation, OutRotation);
}

void UAtlantisInteractorComponent::SetFocus(UAtlantisInteractableComponent* NewFocus)
{
	AActor* Owner = GetOwner();

	UAtlantisInteractableComponent* NewShown = (NewFocus && ShouldShowFocus()) ? NewFocus : nullptr;
	UAtlantisInteractableComponent* OldShown = ShownFocus.Get();
	
	if (OldShown && OldShown != NewShown)
	{
		OldShown->NotifyUnfocused(Owner);
	}
	ShownFocus = NewShown;
	
	if (NewShown && !NewShown->IsFocusedBy(Owner))
	{
		NewShown->NotifyFocused(Owner);
	}

	UAtlantisInteractableComponent* OldFocus = FocusedInteractable.Get();
	if (NewFocus != OldFocus)
	{
		FocusedInteractable = NewFocus;
		OnFocusChanged.Broadcast(NewFocus, OldFocus);
	}
}

bool UAtlantisInteractorComponent::ShouldShowFocus() const
{
	// Highlight is local visual feedback, so ask "is a local human's PlayerController driving this?".
	// (APawn::IsPlayerControlled checks for a non-bot PlayerState instead, which isn't what we mean.)
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const AController* Controller = Pawn ? Pawn->GetController() : nullptr;
	return Controller && Controller->IsLocalPlayerController();
}

void UAtlantisInteractorComponent::TryBindInput()
{
	const AActor* Owner = GetOwner();
	UInputComponent* Input = Owner ? Owner->InputComponent.Get() : nullptr;
	if (!InteractAction || !Input || Input == BoundInputComponent.Get())
	{
		return;
	}

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(Input))
	{
		EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &UAtlantisInteractorComponent::HandleInteractInput);
		BoundInputComponent = Input;
	}
}
