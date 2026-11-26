// Copyright (c) 2026 ASGC

#include "Investigation/AtlantisInspectableComponent.h"
#include "Investigation/AtlantisInspectorComponent.h"
#include "Interaction/AtlantisInteractableComponent.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY_STATIC(LogAtlantisInspectable, Log, All);

UAtlantisInspectableComponent::UAtlantisInspectableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAtlantisInspectableComponent::BeginPlay()
{
	Super::BeginPlay();

	UAtlantisInteractableComponent* Interactable = GetOwner()->FindComponentByClass<UAtlantisInteractableComponent>();
	if (!Interactable)
	{
		UE_LOG(LogAtlantisInspectable, Warning,
			TEXT("%s on %s: no Atlantis Interactable on this actor, so it can never be inspected."),
			*GetName(), *GetNameSafe(GetOwner()));
		return;
	}

	Interactable->OnInteracted.AddDynamic(this, &UAtlantisInspectableComponent::HandleInteracted);
	BoundInteractable = Interactable;
}

void UAtlantisInspectableComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UAtlantisInteractableComponent* Interactable = BoundInteractable.Get())
	{
		Interactable->OnInteracted.RemoveDynamic(this, &UAtlantisInspectableComponent::HandleInteracted);
	}
	BoundInteractable.Reset();

	Super::EndPlay(EndPlayReason);
}

void UAtlantisInspectableComponent::HandleInteracted(AActor* Interactor)
{
	UAtlantisInspectorComponent* Inspector = Interactor ? Interactor->FindComponentByClass<UAtlantisInspectorComponent>() : nullptr;
	if (!Inspector)
	{
		UE_LOG(LogAtlantisInspectable, Warning,
			TEXT("%s on %s: %s has no Atlantis Inspector, so it can't inspect this."),
			*GetName(), *GetNameSafe(GetOwner()), *GetNameSafe(Interactor));
		return;
	}

	Inspector->BeginInspect(this);
}
