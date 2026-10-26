// Copyright (c) 2026 ASGC
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Puzzle/AtlantisPuzzleTypes.h"
#include "AtlantisPuzzleAction.generated.h"

/**
 * Base for anything in a puzzle's On Begin / On Fail / On Complete / On Solved lists. Shows up as a dropdown
 * entry in those lists, configured inline in the Details panel.
 *
 * Add a new action type by subclassing in C++ (override Execute_Implementation) or in Blueprint (override
 * Execute). Set bRunInPreview for actions that only change how things look (visibility, a door's state), so the
 * editor preview buttons run them; sounds, effects and gameplay should leave it off.
 */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, Blueprintable, CollapseCategories)
class PROJECTATLANTIS_API UAtlantisPuzzleAction : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Atlantis|Puzzle")
	void Execute(const FAtlantisPuzzleActionContext& Context);

	/**
	 * Undoes whatever this action changed in the editor preview (the manager's Clear Preview button). Override
	 * alongside bRunInPreview so a custom look action can be cleared too.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Atlantis|Puzzle")
	void ClearPreview(const FAtlantisPuzzleActionContext& Context);

	/** Run this action from the editor preview buttons (Context.bIsPreview is true then). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Preview")
	bool bRunInPreview = false;

	/** Actors whose state this action sets. Used to warn when two tasks set the same actor's start state. */
	virtual void GetAffectedActors(TArray<AActor*>& OutActors) const {}

	/** Setup problems with this entry (e.g. a state its target doesn't have), reported by the manager's warnings. */
	virtual void CollectProblems(TArray<FString>& OutProblems) const {}

	virtual UWorld* GetWorld() const override;
};
