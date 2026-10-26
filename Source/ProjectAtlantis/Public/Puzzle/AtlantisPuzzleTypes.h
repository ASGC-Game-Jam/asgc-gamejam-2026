// Copyright (c) 2026 ASGC
#pragma once

#include "CoreMinimal.h"
#include "AtlantisPuzzleTypes.generated.h"

class AActor;
class AAtlantisPuzzleManager;
class UAtlantisPuzzleAction;

DECLARE_LOG_CATEGORY_EXTERN(LogAtlantisPuzzle, Log, All);

UENUM(BlueprintType)
enum class EAtlantisPuzzleTaskState : uint8
{
	/** Waiting on the tasks it requires. Interacting with it runs its On Fail. */
	Locked,
	/** Can be completed now. */
	Available,
	/** Done. Stays done until the puzzle is reset. */
	Completed
};

/**
 * One thing the player has to do, edited in the puzzle manager's Tasks list.
 *
 * On Begin is this task's part of the level's starting state (applied at game start and on every reset).
 * On Complete is the state it switches to and keeps until the next reset. On Fail is feedback for trying it
 * while it is still locked.
 */
USTRUCT(BlueprintType)
struct PROJECTATLANTIS_API FAtlantisPuzzleTask
{
	GENERATED_BODY()

	/** Designer-facing name. Other tasks pick it in Requires; shown as the list entry's title. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Task")
	FName TaskName;

	/** The actor the player interacts with to complete this task. Empty for tasks completed by another puzzle or a script. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Task")
	TObjectPtr<AActor> InteractWith = nullptr;

	/** Locked until all of these tasks are completed. Empty = available from the start. List order does not matter. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Task", meta = (GetOptions = "GetTaskNames"))
	TArray<FName> Requires;

	/** This task's starting state: applied when the game starts and whenever the puzzle resets. */
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Task")
	TArray<TObjectPtr<UAtlantisPuzzleAction>> OnBegin;

	/** Feedback when the player tries this task while it is locked. Changes nothing else. */
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Task")
	TArray<TObjectPtr<UAtlantisPuzzleAction>> OnFail;

	/** The state this task switches to when completed. It stays until the puzzle resets. */
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Task")
	TArray<TObjectPtr<UAtlantisPuzzleAction>> OnComplete;
};

/** Passed to every action so it knows which puzzle, task and actors triggered it. */
USTRUCT(BlueprintType)
struct PROJECTATLANTIS_API FAtlantisPuzzleActionContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Atlantis|Puzzle")
	TObjectPtr<AAtlantisPuzzleManager> Manager = nullptr;

	/** The task whose list is running, or INDEX_NONE for the puzzle's own On Begin / On Solved. */
	UPROPERTY(BlueprintReadOnly, Category = "Atlantis|Puzzle")
	int32 TaskIndex = INDEX_NONE;

	/** The task's Interact With actor, if any. */
	UPROPERTY(BlueprintReadOnly, Category = "Atlantis|Puzzle")
	TObjectPtr<AActor> TaskActor = nullptr;

	/** Who triggered it, usually the player character. Null when the puzzle itself caused it (start, reset, scripted). */
	UPROPERTY(BlueprintReadOnly, Category = "Atlantis|Puzzle")
	TObjectPtr<AActor> Instigator = nullptr;

	/** True when running from an editor preview button: change only what the designer should see. */
	UPROPERTY(BlueprintReadOnly, Category = "Atlantis|Puzzle")
	bool bIsPreview = false;
};
