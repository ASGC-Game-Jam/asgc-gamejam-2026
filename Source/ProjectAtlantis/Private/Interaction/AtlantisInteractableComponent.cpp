// Copyright (c) 2026 ASGC

#include "Interaction/AtlantisInteractableComponent.h"
#include "Components/MeshComponent.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"

DEFINE_LOG_CATEGORY_STATIC(LogAtlantisInteractable, Log, All);

UAtlantisInteractableComponent::UAtlantisInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAtlantisInteractableComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bHighlightWhenFocused && !HighlightMaterial)
	{
		UE_LOG(LogAtlantisInteractable, Warning,
			TEXT("%s on %s: bHighlightWhenFocused is on but HighlightMaterial is not set, so it will never highlight."),
			*GetName(), *GetNameSafe(GetOwner()));
	}
}

bool UAtlantisInteractableComponent::TryInteract(AActor* Interactor)
{
	if (!CanInteract())
	{
		return false;
	}

	if (bSingleUse)
	{
		SetInteractionEnabled(false);
	}

	OnInteracted.Broadcast(Interactor);
	return true;
}

void UAtlantisInteractableComponent::SetInteractionEnabled(bool bNewEnabled)
{
	bEnabled = bNewEnabled;
	if (!bEnabled)
	{
		const TSet<TWeakObjectPtr<AActor>> Focusing = FocusingInteractors;
		for (const TWeakObjectPtr<AActor>& Interactor : Focusing)
		{
			NotifyUnfocused(Interactor.Get());
		}
		FocusingInteractors.Reset();
		SetHighlighted(false);
	}
}

bool UAtlantisInteractableComponent::IsFocused() const
{
	for (const TWeakObjectPtr<AActor>& Interactor : FocusingInteractors)
	{
		if (Interactor.IsValid())
		{
			return true;
		}
	}
	return false;
}

void UAtlantisInteractableComponent::NotifyFocused(AActor* Interactor)
{
	if (!Interactor || FocusingInteractors.Contains(Interactor))
	{
		return;
	}

	FocusingInteractors.Add(Interactor);
	SetHighlighted(bHighlightWhenFocused);
	OnFocused.Broadcast(Interactor);
}

void UAtlantisInteractableComponent::NotifyUnfocused(AActor* Interactor)
{
	if (!Interactor || !FocusingInteractors.Remove(Interactor))
	{
		return;
	}

	if (!IsFocused())
	{
		SetHighlighted(false);
	}
	OnUnfocused.Broadcast(Interactor);
}

void UAtlantisInteractableComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetHighlighted(false);
	FocusingInteractors.Reset();
	Super::EndPlay(EndPlayReason);
}

void UAtlantisInteractableComponent::SetHighlighted(bool bHighlighted)
{
	if (bHighlighted)
	{
		const AActor* Owner = GetOwner();
		if (!Owner || !HighlightMaterial || !SavedOverlayMaterials.IsEmpty())
		{
			return;
		}

		TArray<UMeshComponent*> Meshes;
		Owner->GetComponents<UMeshComponent>(Meshes);
		for (UMeshComponent* MeshComponent : Meshes)
		{
			SavedOverlayMaterials.Add(MeshComponent, MeshComponent->GetOverlayMaterial());
			MeshComponent->SetOverlayMaterial(HighlightMaterial);
		}
		return;
	}

	for (const TPair<TWeakObjectPtr<UMeshComponent>, TWeakObjectPtr<UMaterialInterface>>& Saved : SavedOverlayMaterials)
	{
		if (UMeshComponent* MeshComponent = Saved.Key.Get())
		{
			MeshComponent->SetOverlayMaterial(Saved.Value.Get());
		}
	}
	SavedOverlayMaterials.Reset();
}
