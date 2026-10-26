// Copyright (c) 2026 ASGC
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AtlantisPuzzleLibrary.generated.h"

class AActor;
class AAtlantisPuzzleManager;

UCLASS()
class PROJECTATLANTIS_API UAtlantisPuzzleLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Call from the player's interact flow with whatever the interaction hit. Routes it to the puzzle
	 * manager whose task uses that actor. Returns true if a puzzle consumed the interaction (completed or
	 * rejected a task), false if no puzzle cares about the actor.
	 */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|Puzzle")
	static bool NotifyPuzzleInteraction(AActor* Target, AActor* Instigator);

	/**
	 * The puzzle manager in Target's world with an unfinished task on Target, preferring one whose task is
	 * available over one whose task is locked. Null if no puzzle uses Target.
	 */
	UFUNCTION(BlueprintPure, Category = "Atlantis|Puzzle")
	static AAtlantisPuzzleManager* FindPuzzleManagerForActor(AActor* Target);
};
