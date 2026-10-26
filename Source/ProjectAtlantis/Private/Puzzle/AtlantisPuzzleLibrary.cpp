// Copyright (c) 2026 ASGC

#include "Puzzle/AtlantisPuzzleLibrary.h"
#include "Puzzle/AtlantisPuzzleManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

bool UAtlantisPuzzleLibrary::NotifyPuzzleInteraction(AActor* Target, AActor* Instigator)
{
	AAtlantisPuzzleManager* Manager = FindPuzzleManagerForActor(Target);
	return Manager && Manager->HandleInteraction(Target, Instigator);
}

AAtlantisPuzzleManager* UAtlantisPuzzleLibrary::FindPuzzleManagerForActor(AActor* Target)
{
	UWorld* World = Target ? Target->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}

	// A level holds a handful of puzzles at most, so a scan is cheaper than keeping a registry in sync.
	// When puzzles share an actor, one that can complete a task beats one that would only reject it.
	AAtlantisPuzzleManager* LockedMatch = nullptr;
	for (TActorIterator<AAtlantisPuzzleManager> It(World); It; ++It)
	{
		const int32 TaskIndex = It->FindOpenTaskFor(Target);
		if (TaskIndex == INDEX_NONE)
		{
			continue;
		}
		if (It->GetTaskState(TaskIndex) == EAtlantisPuzzleTaskState::Available)
		{
			return *It;
		}
		if (!LockedMatch)
		{
			LockedMatch = *It;
		}
	}
	return LockedMatch;
}
