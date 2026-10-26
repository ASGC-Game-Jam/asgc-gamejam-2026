// Copyright (c) 2026 ASGC
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Puzzle/AtlantisPuzzleTypes.h"
#include "AtlantisPuzzleManager.generated.h"

class UAtlantisPuzzleAction;

/** A task changed: became available, was completed, or was attempted while locked. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPuzzleTaskEvent, int32, TaskIndex);

/** Every task is completed. Fires once per run (again only after ResetPuzzle). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPuzzleSolved);

/**
 * Runs one puzzle: a list of tasks, each with On Begin / On Fail / On Complete, plus the puzzle's own On Begin and
 * On Solved. Place one per puzzle and configure it in the Details panel; BP_PuzzleManager is the designer-facing child.
 *
 * At game start (and on every reset) the puzzle's On Begin and every task's On Begin run, setting the level's
 * starting state. A task is available once the tasks it requires are completed; completing it runs its On Complete,
 * which stays until the next reset. When every task is completed, On Solved runs.
 *
 * Interaction reaches the manager through UAtlantisPuzzleLibrary::NotifyPuzzleInteraction, called from the player's
 * interact flow. Not replicated: the game is single-player.
 */
UCLASS(Blueprintable, ClassGroup = (Atlantis))
class PROJECTATLANTIS_API AAtlantisPuzzleManager : public AActor
{
	GENERATED_BODY()

public:
	AAtlantisPuzzleManager();

	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	//~ Designer setup.

	/**
	 * A wrong attempt (interacting with a locked task) puts the whole puzzle back to its start, after that task's
	 * On Fail runs. For sequences where a mistake means starting over, e.g. pressing buttons in a remembered order.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle")
	bool bResetOnFail = false;

	/** The puzzle's own starting state, for things no single task owns (e.g. the door is locked). */
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Puzzle")
	TArray<TObjectPtr<UAtlantisPuzzleAction>> OnBegin;

	/** Runs once when every task is completed (e.g. the door opens). */
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Puzzle")
	TArray<TObjectPtr<UAtlantisPuzzleAction>> OnSolved;

	/** The tasks. Use each task's Requires to decide what must be done first. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle", meta = (TitleProperty = "TaskName"))
	TArray<FAtlantisPuzzleTask> Tasks;

	//~ Change events. Bind from Blueprint, UI, audio or analytics rather than polling.

	UPROPERTY(BlueprintAssignable, Category = "Atlantis|Puzzle")
	FOnPuzzleTaskEvent OnTaskAvailable;

	UPROPERTY(BlueprintAssignable, Category = "Atlantis|Puzzle")
	FOnPuzzleTaskEvent OnTaskCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Atlantis|Puzzle")
	FOnPuzzleTaskEvent OnTaskRejected;

	UPROPERTY(BlueprintAssignable, Category = "Atlantis|Puzzle")
	FOnPuzzleSolved OnPuzzleSolved;

	//~ Runtime.

	/**
	 * Runs every On Begin again and locks every task whose requirements are not met.
	 * Called from inside the puzzle's own reactions (On Complete, On Solved, event listeners), the reset runs as soon
	 * as the current update finishes. Called while a reset is already running (e.g. from On Begin), it is ignored with
	 * a warning, since it would reset forever.
	 */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|Puzzle")
	void ResetPuzzle();

	/**
	 * The player interacted with Target. Completes the matching task if it is available, or runs its On Fail if it is
	 * still locked. Returns true if Target belongs to an unfinished task.
	 */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|Puzzle")
	bool HandleInteraction(AActor* Target, AActor* InteractingActor);

	/** Completes a task directly (chained puzzles, scripted events, debug). Returns false unless the task is available. */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|Puzzle")
	bool TryCompleteTask(int32 TaskIndex, AActor* InteractingActor);

	UFUNCTION(BlueprintPure, Category = "Atlantis|Puzzle")
	EAtlantisPuzzleTaskState GetTaskState(int32 TaskIndex) const;

	UFUNCTION(BlueprintPure, Category = "Atlantis|Puzzle")
	int32 GetTaskCount() const { return Tasks.Num(); }

	UFUNCTION(BlueprintPure, Category = "Atlantis|Puzzle")
	int32 GetCompletedTaskCount() const;

	UFUNCTION(BlueprintPure, Category = "Atlantis|Puzzle")
	bool IsSolved() const { return bSolved; }

	/**
	 * Index of the unfinished task whose Interact With is Actor, preferring an available task over a locked one.
	 * INDEX_NONE if there is none.
	 */
	UFUNCTION(BlueprintPure, Category = "Atlantis|Puzzle")
	int32 FindOpenTaskFor(const AActor* Actor) const;

	/** Names of every task, for the Requires and Preview Task dropdowns. */
	UFUNCTION()
	TArray<FName> GetTaskNames() const;

	/** Index of the first task named TaskName, or INDEX_NONE (always for an empty name). */
	UFUNCTION(BlueprintPure, Category = "Atlantis|Puzzle")
	int32 FindTaskByName(FName TaskName) const;

	/**
	 * Logs a warning for each setup problem: two tasks with the same Interact With, two On Begin lists setting the same
	 * actor, and Requires entries that name no other task. Runs at game start and whenever the manager is edited.
	 * Returns the number of problems found.
	 */
	int32 ValidatePuzzle() const;

	/**
	 * Runtime show/hide for the Set Actors Visible action: rendering and collision together. Hiding remembers the
	 * actor's collision setting and showing restores it, so actors placed without collision never gain it.
	 */
	void SetActorVisible(AActor* Actor, bool bVisible);

	//~ Editor preview. Shows a list's look in the level without pressing Play. Only actions marked Run In Preview
	//~ (visibility, puzzle state) run, and hiding uses editor-only temporary visibility, so nothing saved changes.

#if WITH_EDITORONLY_DATA
	/** The task the Show Task Start / Show Task Completed buttons preview. */
	UPROPERTY(EditInstanceOnly, Transient, Category = "Preview", meta = (GetOptions = "GetTaskNames"))
	FName PreviewTask;
#endif

#if WITH_EDITOR
	/** Shows the chosen Preview Task's On Begin, and nothing else. */
	UFUNCTION(CallInEditor, Category = "Preview", meta = (DisplayName = "Show Task Start"))
	void ShowTaskStart();

	/** Shows the chosen Preview Task's On Complete, and nothing else. */
	UFUNCTION(CallInEditor, Category = "Preview", meta = (DisplayName = "Show Task Completed"))
	void ShowTaskCompleted();

	/** Shows the puzzle's own On Begin. */
	UFUNCTION(CallInEditor, Category = "Preview", meta = (DisplayName = "Show Puzzle Start"))
	void ShowStart();

	/** Shows the puzzle's On Solved. */
	UFUNCTION(CallInEditor, Category = "Preview", meta = (DisplayName = "Show Puzzle Solved"))
	void ShowSolved();

	/** Unhides everything the preview touched and returns puzzle-state targets to their start state. */
	UFUNCTION(CallInEditor, Category = "Preview")
	void ClearPreview();

	/** Runs the preview-safe actions of one list. Used by the preview buttons. */
	void PreviewActions(const TArray<TObjectPtr<UAtlantisPuzzleAction>>& Actions, int32 TaskIndex);
#endif

private:
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Puzzle|Runtime")
	TArray<EAtlantisPuzzleTaskState> TaskStates;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Puzzle|Runtime")
	bool bSolved = false;

	/** Nesting depth of interactions, completions and resets currently running. */
	int32 UpdateDepth = 0;
	bool bResetting = false;
	bool bResetPending = false;

	/** Marks an update for its scope; runs a reset requested during it once the outermost update ends. */
	struct FUpdateScope
	{
		explicit FUpdateScope(AAtlantisPuzzleManager& InManager);
		~FUpdateScope();

		AAtlantisPuzzleManager& Manager;
	};

	/** Each actor's collision setting from before the puzzle first hid it. */
	TMap<TWeakObjectPtr<AActor>, bool> OriginalCollision;

	void ResetNow();
	void CompleteTask(int32 TaskIndex, AActor* InteractingActor);
	void RejectTask(int32 TaskIndex, AActor* InteractingActor);
	void Solve(AActor* InteractingActor);

	/** True when every task the given task requires is completed. */
	bool AreRequirementsMet(int32 TaskIndex) const;

	/** Makes locked tasks whose requirements are met available. */
	void RefreshAvailability();

	/** Number of tasks that have a state; guards against the list being edited after the last reset. */
	int32 TrackedTaskCount() const;

	void RunActions(const TArray<TObjectPtr<UAtlantisPuzzleAction>>& Actions, int32 TaskIndex,
	                AActor* InteractingActor, bool bIsPreview = false);
};
