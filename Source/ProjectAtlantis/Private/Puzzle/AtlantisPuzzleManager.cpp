// Copyright (c) 2026 ASGC

#include "Puzzle/AtlantisPuzzleManager.h"
#include "Puzzle/AtlantisPuzzleAction.h"
#include "Components/BillboardComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "UObject/Script.h"

DEFINE_LOG_CATEGORY(LogAtlantisPuzzle);

AAtlantisPuzzleManager::AAtlantisPuzzleManager()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

#if WITH_EDITORONLY_DATA
	// Gives the manager something to click in the viewport.
	if (UBillboardComponent* Billboard = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Billboard")))
	{
		Billboard->SetupAttachment(RootComponent);
	}
#endif
}

void AAtlantisPuzzleManager::BeginPlay()
{
	Super::BeginPlay();

	ValidatePuzzle();

	// Actors begin play in no fixed order. Waiting a tick lets every actor the On Begin lists touch run its own
	// BeginPlay first, so none of them overwrites the starting state afterwards.
	GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &AAtlantisPuzzleManager::ResetPuzzle));
}

#if WITH_EDITOR
void AAtlantisPuzzleManager::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	ValidatePuzzle();

	const UWorld* World = GetWorld();
	if (World && World->IsGameWorld())
	{
		// Tasks edited in the Details panel during Play: restart so the task states match the new list.
		ResetPuzzle();
	}
}
#endif

AAtlantisPuzzleManager::FUpdateScope::FUpdateScope(AAtlantisPuzzleManager& InManager)
	: Manager(InManager)
{
	++Manager.UpdateDepth;
}

AAtlantisPuzzleManager::FUpdateScope::~FUpdateScope()
{
	if (--Manager.UpdateDepth == 0 && Manager.bResetPending)
	{
		Manager.bResetPending = false;
		Manager.ResetNow();
	}
}

void AAtlantisPuzzleManager::ResetPuzzle()
{
	if (bResetting)
	{
		UE_LOG(LogAtlantisPuzzle, Warning, TEXT("%s: Reset Puzzle ignored because the puzzle is already resetting (e.g. called from On Begin); it would reset forever."),
		       *GetName());
		return;
	}
	if (UpdateDepth > 0)
	{
		bResetPending = true;
		return;
	}
	ResetNow();
}

void AAtlantisPuzzleManager::ResetNow()
{
	FUpdateScope Scope(*this);
	TGuardValue<bool> ResettingGuard(bResetting, true);

	TaskStates.Init(EAtlantisPuzzleTaskState::Locked, Tasks.Num());
	bSolved = false;

	RunActions(OnBegin, INDEX_NONE, nullptr);
	for (int32 Index = 0; Index < Tasks.Num(); ++Index)
	{
		RunActions(Tasks[Index].OnBegin, Index, nullptr);
	}

	RefreshAvailability();

	if (Tasks.Num() > 0 && !TaskStates.Contains(EAtlantisPuzzleTaskState::Available))
	{
		UE_LOG(LogAtlantisPuzzle, Warning, TEXT("%s: no task is available at the start, so the puzzle can never be solved. Check for tasks that require each other."),
		       *GetName());
	}
}

bool AAtlantisPuzzleManager::HandleInteraction(AActor* Target, AActor* InteractingActor)
{
	const int32 TaskIndex = FindOpenTaskFor(Target);
	if (TaskIndex == INDEX_NONE)
	{
		return false;
	}

	FUpdateScope Scope(*this);
	if (TaskStates[TaskIndex] == EAtlantisPuzzleTaskState::Available)
	{
		CompleteTask(TaskIndex, InteractingActor);
	}
	else
	{
		RejectTask(TaskIndex, InteractingActor);
	}
	return true;
}

bool AAtlantisPuzzleManager::TryCompleteTask(int32 TaskIndex, AActor* InteractingActor)
{
	if (!Tasks.IsValidIndex(TaskIndex) || GetTaskState(TaskIndex) != EAtlantisPuzzleTaskState::Available)
	{
		return false;
	}
	FUpdateScope Scope(*this);
	CompleteTask(TaskIndex, InteractingActor);
	return true;
}

EAtlantisPuzzleTaskState AAtlantisPuzzleManager::GetTaskState(int32 TaskIndex) const
{
	// Before the first reset (e.g. a manager that has not begun play) every task reads as locked.
	return TaskStates.IsValidIndex(TaskIndex) ? TaskStates[TaskIndex] : EAtlantisPuzzleTaskState::Locked;
}

int32 AAtlantisPuzzleManager::GetCompletedTaskCount() const
{
	int32 Count = 0;
	for (const EAtlantisPuzzleTaskState State : TaskStates)
	{
		Count += State == EAtlantisPuzzleTaskState::Completed ? 1 : 0;
	}
	return Count;
}

int32 AAtlantisPuzzleManager::FindOpenTaskFor(const AActor* Actor) const
{
	if (!Actor)
	{
		return INDEX_NONE;
	}

	int32 LockedMatch = INDEX_NONE;
	for (int32 Index = 0; Index < TrackedTaskCount(); ++Index)
	{
		if (Tasks[Index].InteractWith != Actor)
		{
			continue;
		}
		if (TaskStates[Index] == EAtlantisPuzzleTaskState::Available)
		{
			return Index;
		}
		if (TaskStates[Index] == EAtlantisPuzzleTaskState::Locked && LockedMatch == INDEX_NONE)
		{
			LockedMatch = Index;
		}
	}
	return LockedMatch;
}

TArray<FName> AAtlantisPuzzleManager::GetTaskNames() const
{
	TArray<FName> Names;
	for (const FAtlantisPuzzleTask& Task : Tasks)
	{
		if (!Task.TaskName.IsNone())
		{
			Names.Add(Task.TaskName);
		}
	}
	return Names;
}

int32 AAtlantisPuzzleManager::FindTaskByName(FName TaskName) const
{
	if (TaskName.IsNone())
	{
		// Unnamed tasks would all match an empty name.
		return INDEX_NONE;
	}
	return Tasks.IndexOfByPredicate([TaskName](const FAtlantisPuzzleTask& Task) { return Task.TaskName == TaskName; });
}

int32 AAtlantisPuzzleManager::ValidatePuzzle() const
{
	int32 Problems = 0;
	auto Describe = [this](int32 Index)
	{
		return FString::Printf(TEXT("task %d (%s)"), Index, *Tasks[Index].TaskName.ToString());
	};

	TMap<const AActor*, int32> InteractOwners;
	TMap<const AActor*, FString> BeginOwners;
	auto ClaimBegin = [&](const TArray<TObjectPtr<UAtlantisPuzzleAction>>& Actions, const FString& ListOwner)
	{
		TArray<AActor*> Affected;
		for (const UAtlantisPuzzleAction* Action : Actions)
		{
			if (Action)
			{
				Action->GetAffectedActors(Affected);
			}
		}
		for (const AActor* Actor : Affected)
		{
			if (const FString* Existing = BeginOwners.Find(Actor))
			{
				if (*Existing != ListOwner)
				{
					UE_LOG(LogAtlantisPuzzle, Warning, TEXT("%s: %s and %s both set %s in On Begin; the later one wins at game start. Set each actor's start state once."),
					       *GetName(), **Existing, *ListOwner, *Actor->GetActorNameOrLabel());
					++Problems;
				}
				continue;
			}
			BeginOwners.Add(Actor, ListOwner);
		}
	};

	auto CheckEntries = [&](const TArray<TObjectPtr<UAtlantisPuzzleAction>>& Actions, const FString& ListName)
	{
		for (const UAtlantisPuzzleAction* Action : Actions)
		{
			if (!Action)
			{
				continue;
			}
			TArray<FString> EntryProblems;
			Action->CollectProblems(EntryProblems);
			for (const FString& Problem : EntryProblems)
			{
				UE_LOG(LogAtlantisPuzzle, Warning, TEXT("%s: %s: %s."), *GetName(), *ListName, *Problem);
				++Problems;
			}
		}
	};
	CheckEntries(OnBegin, TEXT("the puzzle's On Begin"));
	CheckEntries(OnSolved, TEXT("the puzzle's On Solved"));
	for (int32 Index = 0; Index < Tasks.Num(); ++Index)
	{
		CheckEntries(Tasks[Index].OnBegin, Describe(Index) + TEXT(" On Begin"));
		CheckEntries(Tasks[Index].OnFail, Describe(Index) + TEXT(" On Fail"));
		CheckEntries(Tasks[Index].OnComplete, Describe(Index) + TEXT(" On Complete"));
	}

	TMap<FName, int32> NameOwners;
	ClaimBegin(OnBegin, TEXT("the puzzle"));
	for (int32 Index = 0; Index < Tasks.Num(); ++Index)
	{
		const FAtlantisPuzzleTask& Task = Tasks[Index];

		if (!Task.TaskName.IsNone())
		{
			if (const int32* Other = NameOwners.Find(Task.TaskName))
			{
				UE_LOG(LogAtlantisPuzzle, Warning, TEXT("%s: %s and %s have the same Task Name; Requires and Complete Puzzle Task only ever find the first. Rename one."),
				       *GetName(), *Describe(*Other), *Describe(Index));
				++Problems;
			}
			else
			{
				NameOwners.Add(Task.TaskName, Index);
			}
		}

		if (Task.InteractWith)
		{
			if (const int32* Other = InteractOwners.Find(Task.InteractWith))
			{
				UE_LOG(LogAtlantisPuzzle, Warning, TEXT("%s: %s and %s both use %s as Interact With; a press goes to whichever is open first."),
				       *GetName(), *Describe(*Other), *Describe(Index), *Task.InteractWith->GetActorNameOrLabel());
				++Problems;
			}
			else
			{
				InteractOwners.Add(Task.InteractWith, Index);
			}
		}

		ClaimBegin(Task.OnBegin, Describe(Index));

		for (const FName& Required : Task.Requires)
		{
			if (Required.IsNone())
			{
				// A Requires entry that has just been added and not picked yet; ignored until it has a name.
				continue;
			}
			const int32 RequiredIndex = FindTaskByName(Required);
			if (RequiredIndex == INDEX_NONE || RequiredIndex == Index)
			{
				UE_LOG(LogAtlantisPuzzle, Warning, TEXT("%s: %s requires '%s', which is not another task in this puzzle; that requirement is ignored."),
				       *GetName(), *Describe(Index), *Required.ToString());
				++Problems;
			}
		}
	}
	return Problems;
}

void AAtlantisPuzzleManager::CompleteTask(int32 TaskIndex, AActor* InteractingActor)
{
	TaskStates[TaskIndex] = EAtlantisPuzzleTaskState::Completed;
	RunActions(Tasks[TaskIndex].OnComplete, TaskIndex, InteractingActor);
	OnTaskCompleted.Broadcast(TaskIndex);

	if (GetCompletedTaskCount() == Tasks.Num())
	{
		Solve(InteractingActor);
	}
	else
	{
		RefreshAvailability();
	}
}

void AAtlantisPuzzleManager::RejectTask(int32 TaskIndex, AActor* InteractingActor)
{
	RunActions(Tasks[TaskIndex].OnFail, TaskIndex, InteractingActor);
	OnTaskRejected.Broadcast(TaskIndex);

	if (bResetOnFail && GetCompletedTaskCount() > 0)
	{
		ResetPuzzle();
	}
}

void AAtlantisPuzzleManager::Solve(AActor* InteractingActor)
{
	if (bSolved)
	{
		return;
	}
	bSolved = true;
	RunActions(OnSolved, INDEX_NONE, InteractingActor);
	OnPuzzleSolved.Broadcast();
}

bool AAtlantisPuzzleManager::AreRequirementsMet(int32 TaskIndex) const
{
	for (const FName& Required : Tasks[TaskIndex].Requires)
	{
		const int32 RequiredIndex = FindTaskByName(Required);
		// Unknown names and self-references are reported by ValidatePuzzle and ignored here.
		if (RequiredIndex != INDEX_NONE && RequiredIndex != TaskIndex
			&& GetTaskState(RequiredIndex) != EAtlantisPuzzleTaskState::Completed)
		{
			return false;
		}
	}
	return true;
}

int32 AAtlantisPuzzleManager::TrackedTaskCount() const
{
	// Tasks and their states only differ in length between an edit of the list and the next reset.
	return FMath::Min(Tasks.Num(), TaskStates.Num());
}

void AAtlantisPuzzleManager::RefreshAvailability()
{
	for (int32 Index = 0; Index < TrackedTaskCount(); ++Index)
	{
		if (TaskStates[Index] == EAtlantisPuzzleTaskState::Locked && AreRequirementsMet(Index))
		{
			TaskStates[Index] = EAtlantisPuzzleTaskState::Available;
			OnTaskAvailable.Broadcast(Index);
		}
	}
}

void AAtlantisPuzzleManager::SetActorVisible(AActor* Actor, bool bVisible)
{
	if (!Actor)
	{
		return;
	}
	if (bVisible)
	{
		Actor->SetActorHiddenInGame(false);
		// Only restore what the puzzle itself turned off; an actor it never hid keeps its collision as is.
		if (const bool* bHadCollision = OriginalCollision.Find(Actor))
		{
			Actor->SetActorEnableCollision(*bHadCollision);
		}
		return;
	}
	if (!OriginalCollision.Contains(Actor))
	{
		OriginalCollision.Add(Actor, Actor->GetActorEnableCollision());
	}
	Actor->SetActorHiddenInGame(true);
	Actor->SetActorEnableCollision(false);
}

void AAtlantisPuzzleManager::RunActions(const TArray<TObjectPtr<UAtlantisPuzzleAction>>& Actions, int32 TaskIndex,
                                        AActor* InteractingActor, bool bIsPreview)
{
	FAtlantisPuzzleActionContext Context;
	Context.Manager = this;
	Context.TaskIndex = TaskIndex;
	Context.TaskActor = Tasks.IsValidIndex(TaskIndex) ? Tasks[TaskIndex].InteractWith : nullptr;
	Context.Instigator = InteractingActor;
	Context.bIsPreview = bIsPreview;

	for (UAtlantisPuzzleAction* Action : Actions)
	{
		if (Action && (!bIsPreview || Action->bRunInPreview))
		{
			Action->Execute(Context);
		}
	}
}

#if WITH_EDITOR
void AAtlantisPuzzleManager::ShowTaskStart()
{
	const int32 Index = FindTaskByName(PreviewTask);
	if (Index == INDEX_NONE)
	{
		UE_LOG(LogAtlantisPuzzle, Warning, TEXT("%s: pick a task in Preview Task first."), *GetName());
		return;
	}
	PreviewActions(Tasks[Index].OnBegin, Index);
}

void AAtlantisPuzzleManager::ShowTaskCompleted()
{
	const int32 Index = FindTaskByName(PreviewTask);
	if (Index == INDEX_NONE)
	{
		UE_LOG(LogAtlantisPuzzle, Warning, TEXT("%s: pick a task in Preview Task first."), *GetName());
		return;
	}
	PreviewActions(Tasks[Index].OnComplete, Index);
}

void AAtlantisPuzzleManager::ShowStart()
{
	PreviewActions(OnBegin, INDEX_NONE);
}

void AAtlantisPuzzleManager::ShowSolved()
{
	PreviewActions(OnSolved, INDEX_NONE);
}

void AAtlantisPuzzleManager::PreviewActions(const TArray<TObjectPtr<UAtlantisPuzzleAction>>& Actions, int32 TaskIndex)
{
	const UWorld* World = GetWorld();
	if (World && World->IsGameWorld())
	{
		// During play the real state owns the level.
		return;
	}

	// Editor worlds block Blueprint calls on actors; puzzle-state targets like a Blueprint door need them.
	FEditorScriptExecutionGuard ScriptGuard;
	RunActions(Actions, TaskIndex, nullptr, /*bIsPreview*/ true);
}

void AAtlantisPuzzleManager::ClearPreview()
{
	const UWorld* World = GetWorld();
	if (World && World->IsGameWorld())
	{
		return;
	}

	FEditorScriptExecutionGuard ScriptGuard;
	auto Clear = [this](const TArray<TObjectPtr<UAtlantisPuzzleAction>>& Actions, int32 TaskIndex)
	{
		FAtlantisPuzzleActionContext Context;
		Context.Manager = this;
		Context.TaskIndex = TaskIndex;
		Context.TaskActor = Tasks.IsValidIndex(TaskIndex) ? Tasks[TaskIndex].InteractWith : nullptr;
		Context.bIsPreview = true;
		for (UAtlantisPuzzleAction* Action : Actions)
		{
			if (Action && Action->bRunInPreview)
			{
				Action->ClearPreview(Context);
			}
		}
	};

	Clear(OnBegin, INDEX_NONE);
	Clear(OnSolved, INDEX_NONE);
	for (int32 Index = 0; Index < Tasks.Num(); ++Index)
	{
		Clear(Tasks[Index].OnBegin, Index);
		Clear(Tasks[Index].OnFail, Index);
		Clear(Tasks[Index].OnComplete, Index);
	}
}
#endif
