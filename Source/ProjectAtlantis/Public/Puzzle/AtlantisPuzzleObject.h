// Copyright (c) 2026 ASGC
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AtlantisPuzzleObject.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UAtlantisPuzzleObject : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implement on any actor that has states of its own a puzzle can put it in, e.g. a door (Locked / Unlocked /
 * Membrane) or a valve (Closed / Half / Open). The actor lists its states; designers pick one from a dropdown in a
 * Set Object State entry; the actor is told which state to be in and reacts.
 */
class PROJECTATLANTIS_API IAtlantisPuzzleObject
{
	GENERATED_BODY()

public:
	/**
	 * The states this actor can be put in, as shown in Set Object State's dropdown. List the default state first:
	 * the editor's Clear Preview returns the actor to it.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Atlantis|Puzzle")
	TArray<FName> GetPuzzleStates() const;

	/**
	 * Switch to State, one of the names from Get Puzzle States.
	 * @param bIsPreview  true when called from the editor preview buttons; change only what the designer should see
	 *                    and skip sounds and gameplay.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Atlantis|Puzzle")
	void OnPuzzleState(FName State, bool bIsPreview);
};
