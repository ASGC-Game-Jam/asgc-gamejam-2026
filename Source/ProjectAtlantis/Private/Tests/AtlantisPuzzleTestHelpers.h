// Copyright (c) 2026 ASGC

#pragma once

#include "GameFramework/Actor.h"
#include "Puzzle/AtlantisPuzzleAction.h"
#include "Puzzle/AtlantisPuzzleManager.h"
#include "Puzzle/AtlantisPuzzleObject.h"
#include "AtlantisPuzzleTestHelpers.generated.h"

/** Counts how often a puzzle ran it and remembers the last context. */
UCLASS(Transient, NotBlueprintable, HideDropdown)
class UAtlantisPuzzleTestAction : public UAtlantisPuzzleAction
{
	GENERATED_BODY()

public:
	int32 ExecuteCount = 0;
	FAtlantisPuzzleActionContext LastContext;

	virtual void Execute_Implementation(const FAtlantisPuzzleActionContext& Context) override
	{
		++ExecuteCount;
		LastContext = Context;
	}
};

/** A custom look action that takes part in the editor preview, counting preview runs and clears. */
UCLASS(Transient, NotBlueprintable, HideDropdown)
class UAtlantisPuzzleTestPreviewAction : public UAtlantisPuzzleAction
{
	GENERATED_BODY()

public:
	int32 PreviewCount = 0;
	int32 ClearCount = 0;

	UAtlantisPuzzleTestPreviewAction() { bRunInPreview = true; }

	virtual void Execute_Implementation(const FAtlantisPuzzleActionContext& Context) override
	{
		PreviewCount += Context.bIsPreview ? 1 : 0;
	}

	virtual void ClearPreview_Implementation(const FAtlantisPuzzleActionContext& Context) override
	{
		++ClearCount;
	}
};

/** Asks the puzzle that ran it to reset, like a designer's "reset" action would. */
UCLASS(Transient, NotBlueprintable, HideDropdown)
class UAtlantisPuzzleTestResetAction : public UAtlantisPuzzleAction
{
	GENERATED_BODY()

public:
	int32 ExecuteCount = 0;

	virtual void Execute_Implementation(const FAtlantisPuzzleActionContext& Context) override
	{
		++ExecuteCount;
		if (Context.Manager)
		{
			Context.Manager->ResetPuzzle();
		}
	}
};

/** Stand-in for a door: declares Locked (default) and Unlocked, and records the last state it was put in. */
UCLASS(Transient, NotBlueprintable, NotPlaceable, HideDropdown)
class AAtlantisPuzzleTestDoor : public AActor, public IAtlantisPuzzleObject
{
	GENERATED_BODY()

public:
	int32 ApplyCount = 0;
	FName LastState;
	bool bLastPreview = false;

	virtual TArray<FName> GetPuzzleStates_Implementation() const override
	{
		return {TEXT("Locked"), TEXT("Unlocked")};
	}

	virtual void OnPuzzleState_Implementation(FName State, bool bIsPreview) override
	{
		++ApplyCount;
		LastState = State;
		bLastPreview = bIsPreview;
	}
};

/** Records the manager's dynamic delegates. */
UCLASS(Transient)
class UAtlantisPuzzleTestListener : public UObject
{
	GENERATED_BODY()

public:
	TArray<int32> Available;
	TArray<int32> Completed;
	TArray<int32> Rejected;
	int32 SolvedCount = 0;

	UFUNCTION()
	void RecordAvailable(int32 TaskIndex) { Available.Add(TaskIndex); }

	UFUNCTION()
	void RecordCompleted(int32 TaskIndex) { Completed.Add(TaskIndex); }

	UFUNCTION()
	void RecordRejected(int32 TaskIndex) { Rejected.Add(TaskIndex); }

	UFUNCTION()
	void RecordSolved() { ++SolvedCount; }
};
