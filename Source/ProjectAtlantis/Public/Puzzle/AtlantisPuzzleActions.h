// Copyright (c) 2026 ASGC
#pragma once

#include "CoreMinimal.h"
#include "Puzzle/AtlantisPuzzleAction.h"
#include "AtlantisPuzzleActions.generated.h"

class AAtlantisPuzzleManager;
class USoundBase;
class UActorComponent;

/**
 * Shows or hides actors (rendering and collision), and by default everything attached to them, like a parent and
 * its children in the Outliner. Runs in the editor preview.
 */
UCLASS(meta = (DisplayName = "Set Actors Visible"))
class PROJECTATLANTIS_API UAtlantisPuzzleAction_SetActorsVisible : public UAtlantisPuzzleAction
{
	GENERATED_BODY()

public:
	UAtlantisPuzzleAction_SetActorsVisible();

	virtual void Execute_Implementation(const FAtlantisPuzzleActionContext& Context) override;
	virtual void ClearPreview_Implementation(const FAtlantisPuzzleActionContext& Context) override;
	virtual void GetAffectedActors(TArray<AActor*>& OutActors) const override;

	/** The targets, plus everything attached to them when Include Attached Actors is on. */
	TArray<AActor*> GetActorsToChange() const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visibility")
	TArray<TObjectPtr<AActor>> Targets;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visibility")
	bool bVisible = true;

	/** Also show or hide every actor attached to the targets (the ones nested under them in the Outliner). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visibility")
	bool bIncludeAttachedActors = true;
};

/**
 * Puts actors that implement Atlantis Puzzle Object (e.g. a door) into one of their own named states, picked from a
 * dropdown of the states the targets list (e.g. Locked / Unlocked / Membrane). Runs in the editor preview.
 */
UCLASS(meta = (DisplayName = "Set Object State"))
class PROJECTATLANTIS_API UAtlantisPuzzleAction_SetObjectState : public UAtlantisPuzzleAction
{
	GENERATED_BODY()

public:
	UAtlantisPuzzleAction_SetObjectState();

	virtual void Execute_Implementation(const FAtlantisPuzzleActionContext& Context) override;
	virtual void ClearPreview_Implementation(const FAtlantisPuzzleActionContext& Context) override;
	virtual void GetAffectedActors(TArray<AActor*>& OutActors) const override;
	virtual void CollectProblems(TArray<FString>& OutProblems) const override;

	/** States every target lists, for the State dropdown. */
	UFUNCTION()
	TArray<FName> GetStateOptions() const;

	/** Actors that implement Atlantis Puzzle Object. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State")
	TArray<TObjectPtr<AActor>> Targets;

	/** The state to put the targets in. The dropdown lists the states the targets declare. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State", meta = (GetOptions = "GetStateOptions"))
	FName State;
};

/** Plays a sound at the task's actor, at another actor, or in 2D. */
UCLASS(meta = (DisplayName = "Play Sound"))
class PROJECTATLANTIS_API UAtlantisPuzzleAction_PlaySound : public UAtlantisPuzzleAction
{
	GENERATED_BODY()

public:
	virtual void Execute_Implementation(const FAtlantisPuzzleActionContext& Context) override;

	/** Play At if set, else the task's Interact With, else the puzzle manager (puzzle-level lists, tasks without one). */
	const AActor* GetSoundLocation(const FAtlantisPuzzleActionContext& Context) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundBase> Sound = nullptr;

	/** Plays without a location (UI-style). Ignores Play At. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	bool bPlay2D = false;

	/** Where to play the sound. Empty uses the task's Interact With actor, or the puzzle manager if there is none. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound", meta = (EditCondition = "!bPlay2D"))
	TObjectPtr<AActor> PlayAt = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound", meta = (ClampMin = "0.0"))
	float VolumeMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound", meta = (ClampMin = "0.0"))
	float PitchMultiplier = 1.f;
};

/**
 * Completes a task on another puzzle manager. Chains puzzles: solving a smaller puzzle (realign the drive in the
 * lower works) completes a task in a larger one (the engine hall). Leave the target task's Interact With empty so it
 * can only be completed this way.
 */
UCLASS(meta = (DisplayName = "Complete Puzzle Task"))
class PROJECTATLANTIS_API UAtlantisPuzzleAction_CompletePuzzleTask : public UAtlantisPuzzleAction
{
	GENERATED_BODY()

public:
	virtual void Execute_Implementation(const FAtlantisPuzzleActionContext& Context) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<AAtlantisPuzzleManager> Puzzle = nullptr;

	/** Matches the target task's Task Name. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle")
	FName TaskName;
};

/** Activates or deactivates components on actors, e.g. turning a smoke or bubble effect on or off. */
UCLASS(meta = (DisplayName = "Set Components Active"))
class PROJECTATLANTIS_API UAtlantisPuzzleAction_SetComponentsActive : public UAtlantisPuzzleAction
{
	GENERATED_BODY()

public:
	UAtlantisPuzzleAction_SetComponentsActive();

	virtual void Execute_Implementation(const FAtlantisPuzzleActionContext& Context) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	TArray<TObjectPtr<AActor>> Targets;

	/** Which components on the targets to affect. Defaults to effects (Niagara and Cascade). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	TSubclassOf<UActorComponent> ComponentClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	bool bActive = false;
};
