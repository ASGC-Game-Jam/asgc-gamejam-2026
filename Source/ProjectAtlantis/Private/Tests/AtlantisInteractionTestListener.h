// Copyright (c) 2026 ASGC

#pragma once

#include "Interaction/AtlantisInteractableComponent.h"
#include "AtlantisInteractionTestListener.generated.h"

/** Records the real dynamic delegates emitted by the interaction components during automation tests. */
UCLASS(Transient)
class UAtlantisInteractionTestListener : public UObject
{
	GENERATED_BODY()

public:
	int32 InteractedCount = 0;
	int32 FocusedCount = 0;
	int32 UnfocusedCount = 0;
	int32 FocusChangedCount = 0;
	TWeakObjectPtr<AActor> LastInteractor;
	TWeakObjectPtr<UAtlantisInteractableComponent> LastNewFocus;

	UFUNCTION()
	void RecordInteracted(AActor* Interactor)
	{
		++InteractedCount;
		LastInteractor = Interactor;
	}

	UFUNCTION()
	void RecordFocused(AActor* Interactor) { ++FocusedCount; }

	UFUNCTION()
	void RecordUnfocused(AActor* Interactor) { ++UnfocusedCount; }

	UFUNCTION()
	void RecordFocusChanged(UAtlantisInteractableComponent* NewFocus, UAtlantisInteractableComponent* OldFocus)
	{
		++FocusChangedCount;
		LastNewFocus = NewFocus;
	}
};
