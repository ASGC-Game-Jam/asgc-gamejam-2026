// Copyright (c) 2026 ASGC

#include "Puzzle/AtlantisPuzzleAction.h"
#include "GameFramework/Actor.h"

void UAtlantisPuzzleAction::Execute_Implementation(const FAtlantisPuzzleActionContext& Context)
{
}

void UAtlantisPuzzleAction::ClearPreview_Implementation(const FAtlantisPuzzleActionContext& Context)
{
}

UWorld* UAtlantisPuzzleAction::GetWorld() const
{
	// Instanced actions are outered to the manager actor; returning its world lets Blueprint
	// subclasses use world-context nodes (spawn, sound, timers).
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return nullptr;
	}
	const AActor* OwningActor = GetTypedOuter<AActor>();
	return OwningActor ? OwningActor->GetWorld() : nullptr;
}
