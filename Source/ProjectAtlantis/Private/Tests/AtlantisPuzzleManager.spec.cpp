// Copyright (c) 2026 ASGC

#if WITH_DEV_AUTOMATION_TESTS

#include "Puzzle/AtlantisPuzzleActions.h"
#include "Puzzle/AtlantisPuzzleLibrary.h"
#include "Puzzle/AtlantisPuzzleManager.h"
#include "Tests/AtlantisPuzzleTestHelpers.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "UObject/Script.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	constexpr int32 TaskCount = 3;

	FAtlantisPuzzleTask& AddTask(AAtlantisPuzzleManager* Manager, FName Name, AActor* InteractWith, TArray<FName> Requires = {})
	{
		FAtlantisPuzzleTask& Task = Manager->Tasks.AddDefaulted_GetRef();
		Task.TaskName = Name;
		Task.InteractWith = InteractWith;
		Task.Requires = MoveTemp(Requires);
		return Task;
	}

	UAtlantisPuzzleAction_SetActorsVisible* Visible(UObject* Outer, std::initializer_list<AActor*> Targets, bool bVisible)
	{
		UAtlantisPuzzleAction_SetActorsVisible* Action = NewObject<UAtlantisPuzzleAction_SetActorsVisible>(Outer);
		for (AActor* Target : Targets)
		{
			Action->Targets.Add(Target);
		}
		Action->bVisible = bVisible;
		return Action;
	}

	UAtlantisPuzzleAction_SetObjectState* State(UObject* Outer, AActor* Target, FName StateName)
	{
		UAtlantisPuzzleAction_SetObjectState* Action = NewObject<UAtlantisPuzzleAction_SetObjectState>(Outer);
		Action->Targets.Add(Target);
		Action->State = StateName;
		return Action;
	}

	bool IsShown(const AActor* Actor) { return Actor && !Actor->IsHidden() && Actor->GetActorEnableCollision(); }

	// A three-task puzzle in its own runtime world. Each task swaps a "before" actor for an "after" actor
	// (On Begin shows before / hides after, On Complete the reverse) and has counting actions on every list.
	// The puzzle locks a stateful "door" in its On Begin and opens it in On Solved.
	struct FAtlantisPuzzleFixture
	{
		// The test world never initializes actors for play, which makes AActor::ProcessEvent drop interface
		// calls on the stateful door. Real game worlds initialize actors, so allow script execution here.
		FEditorScriptExecutionGuard ScriptGuard;
		TStrongObjectPtr<UWorld> World;
		AAtlantisPuzzleManager* Manager = nullptr;
		AActor* Interact[TaskCount] = {};
		AActor* Before[TaskCount] = {};
		AActor* After[TaskCount] = {};
		UAtlantisPuzzleTestAction* Begin[TaskCount] = {};
		UAtlantisPuzzleTestAction* Fail[TaskCount] = {};
		UAtlantisPuzzleTestAction* Complete[TaskCount] = {};
		UAtlantisPuzzleTestAction* Solved = nullptr;
		AAtlantisPuzzleTestDoor* Door = nullptr;
		AActor* Unrelated = nullptr;
		TStrongObjectPtr<UAtlantisPuzzleTestListener> Listener;

		explicit FAtlantisPuzzleFixture(bool bSequence = true)
			: World(UWorld::CreateWorld(EWorldType::Game, false)),
			  Listener(NewObject<UAtlantisPuzzleTestListener>())
		{
			if (!World.IsValid())
			{
				return;
			}

			FActorSpawnParameters SpawnParameters;
			SpawnParameters.ObjectFlags |= RF_Transient;
			Manager = World->SpawnActor<AAtlantisPuzzleManager>(SpawnParameters);
			Door = World->SpawnActor<AAtlantisPuzzleTestDoor>(SpawnParameters);
			Unrelated = World->SpawnActor<AActor>(SpawnParameters);
			if (!Manager)
			{
				return;
			}

			Manager->OnBegin.Add(State(Manager, Door, TEXT("Locked")));
			Solved = NewObject<UAtlantisPuzzleTestAction>(Manager);
			Manager->OnSolved.Add(State(Manager, Door, TEXT("Unlocked")));
			Manager->OnSolved.Add(Solved);

			for (int32 Index = 0; Index < TaskCount; ++Index)
			{
				Interact[Index] = World->SpawnActor<AActor>(SpawnParameters);
				Before[Index] = World->SpawnActor<AActor>(SpawnParameters);
				After[Index] = World->SpawnActor<AActor>(SpawnParameters);

				FAtlantisPuzzleTask& NewTask = AddTask(Manager, *FString::Printf(TEXT("Task%d"), Index), Interact[Index], bSequence && Index > 0 ? TArray<FName>{*FString::Printf(TEXT("Task%d"), Index - 1)} : TArray<FName>{});
				Begin[Index] = NewObject<UAtlantisPuzzleTestAction>(Manager);
				Fail[Index] = NewObject<UAtlantisPuzzleTestAction>(Manager);
				Complete[Index] = NewObject<UAtlantisPuzzleTestAction>(Manager);
				NewTask.OnBegin = {Visible(Manager, {Before[Index]}, true), Visible(Manager, {After[Index]}, false), Begin[Index]};
				NewTask.OnFail = {Fail[Index]};
				NewTask.OnComplete = {Visible(Manager, {Before[Index]}, false), Visible(Manager, {After[Index]}, true), Complete[Index]};
			}

			Manager->OnTaskAvailable.AddDynamic(Listener.Get(), &UAtlantisPuzzleTestListener::RecordAvailable);
			Manager->OnTaskCompleted.AddDynamic(Listener.Get(), &UAtlantisPuzzleTestListener::RecordCompleted);
			Manager->OnTaskRejected.AddDynamic(Listener.Get(), &UAtlantisPuzzleTestListener::RecordRejected);
			Manager->OnPuzzleSolved.AddDynamic(Listener.Get(), &UAtlantisPuzzleTestListener::RecordSolved);

			Manager->ResetPuzzle();
		}

		~FAtlantisPuzzleFixture()
		{
			if (World.IsValid())
			{
				World->DestroyWorld(false);
			}
		}

		bool IsValid() const { return World.IsValid() && Manager && Door && Unrelated; }
	};
}

BEGIN_DEFINE_SPEC(FAtlantisPuzzleManagerSpec, "Atlantis.Puzzle.Manager",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	TUniquePtr<FAtlantisPuzzleFixture> Fixture;
END_DEFINE_SPEC(FAtlantisPuzzleManagerSpec)

void FAtlantisPuzzleManagerSpec::Define()
{
	AfterEach([this]()
	{
		Fixture.Reset();
	});

	Describe(TEXT("Sequence"), [this]()
	{
		BeforeEach([this]()
		{
			Fixture = MakeUnique<FAtlantisPuzzleFixture>(true);
			TestTrue(TEXT("Fixture built"), Fixture->IsValid());
		});

		It(TEXT("should run every On Begin at start and open only tasks without requirements"), [this]()
		{
			if (!Fixture->IsValid())
			{
				return;
			}
			TestTrue(TEXT("Task 0 available"), Fixture->Manager->GetTaskState(0) == EAtlantisPuzzleTaskState::Available);
			TestTrue(TEXT("Task 1 locked"), Fixture->Manager->GetTaskState(1) == EAtlantisPuzzleTaskState::Locked);
			TestTrue(TEXT("Task 2 locked"), Fixture->Manager->GetTaskState(2) == EAtlantisPuzzleTaskState::Locked);
			for (int32 Index = 0; Index < TaskCount; ++Index)
			{
				TestEqual(TEXT("Every task's On Begin ran, locked or not"), Fixture->Begin[Index]->ExecuteCount, 1);
				TestTrue(TEXT("Before shown"), IsShown(Fixture->Before[Index]));
				TestFalse(TEXT("After hidden"), IsShown(Fixture->After[Index]));
			}
			TestTrue(TEXT("Puzzle On Begin locked the door"), Fixture->Door->ApplyCount == 1 && Fixture->Door->LastState == TEXT("Locked") && !Fixture->Door->bLastPreview);
			TestTrue(TEXT("One available event"), Fixture->Listener->Available == TArray<int32>{0});
		});

		It(TEXT("should complete the available task, keep its On Complete state, and open the next"), [this]()
		{
			if (!Fixture->IsValid())
			{
				return;
			}
			TestTrue(TEXT("Interaction consumed"), Fixture->Manager->HandleInteraction(Fixture->Interact[0], Fixture->Unrelated));
			TestTrue(TEXT("Task 0 completed"), Fixture->Manager->GetTaskState(0) == EAtlantisPuzzleTaskState::Completed);
			TestTrue(TEXT("Task 1 available"), Fixture->Manager->GetTaskState(1) == EAtlantisPuzzleTaskState::Available);
			TestFalse(TEXT("Task 0 before hidden"), IsShown(Fixture->Before[0]));
			TestTrue(TEXT("Task 0 after shown"), IsShown(Fixture->After[0]));
			TestTrue(TEXT("Task 1 untouched"), IsShown(Fixture->Before[1]) && !IsShown(Fixture->After[1]));
			TestEqual(TEXT("On Complete action ran once"), Fixture->Complete[0]->ExecuteCount, 1);
			TestEqual(TEXT("Context carries the task index"), Fixture->Complete[0]->LastContext.TaskIndex, 0);
			TestTrue(TEXT("Context carries the task actor"), Fixture->Complete[0]->LastContext.TaskActor == Fixture->Interact[0]);
			TestTrue(TEXT("Context carries the instigator"), Fixture->Complete[0]->LastContext.Instigator == Fixture->Unrelated);
			TestEqual(TEXT("On Begin did not run again"), Fixture->Begin[1]->ExecuteCount, 1);
		});

		It(TEXT("should run On Fail for a locked task and change nothing"), [this]()
		{
			if (!Fixture->IsValid())
			{
				return;
			}
			TestTrue(TEXT("Locked interaction consumed"), Fixture->Manager->HandleInteraction(Fixture->Interact[2], nullptr));
			TestEqual(TEXT("On Fail ran"), Fixture->Fail[2]->ExecuteCount, 1);
			TestTrue(TEXT("Still locked"), Fixture->Manager->GetTaskState(2) == EAtlantisPuzzleTaskState::Locked);
			TestTrue(TEXT("Looks unchanged"), IsShown(Fixture->Before[2]) && !IsShown(Fixture->After[2]));
			TestTrue(TEXT("Rejected event"), Fixture->Listener->Rejected == TArray<int32>{2});
		});

		It(TEXT("should ignore unrelated actors and completed tasks"), [this]()
		{
			if (!Fixture->IsValid())
			{
				return;
			}
			TestFalse(TEXT("Unrelated actor not consumed"), Fixture->Manager->HandleInteraction(Fixture->Unrelated, nullptr));
			TestFalse(TEXT("Null not consumed"), Fixture->Manager->HandleInteraction(nullptr, nullptr));
			Fixture->Manager->HandleInteraction(Fixture->Interact[0], nullptr);
			TestFalse(TEXT("Completed task not consumed again"), Fixture->Manager->HandleInteraction(Fixture->Interact[0], nullptr));
			TestEqual(TEXT("On Complete ran only once"), Fixture->Complete[0]->ExecuteCount, 1);
		});

		It(TEXT("should solve once, run On Solved, and open the door"), [this]()
		{
			if (!Fixture->IsValid())
			{
				return;
			}
			for (int32 Index = 0; Index < TaskCount; ++Index)
			{
				TestFalse(TEXT("Not solved before the last task"), Fixture->Manager->IsSolved());
				Fixture->Manager->HandleInteraction(Fixture->Interact[Index], nullptr);
			}
			TestTrue(TEXT("Solved"), Fixture->Manager->IsSolved());
			TestEqual(TEXT("Solved event once"), Fixture->Listener->SolvedCount, 1);
			TestEqual(TEXT("On Solved action ran once"), Fixture->Solved->ExecuteCount, 1);
			TestEqual(TEXT("On Solved has no task"), Fixture->Solved->LastContext.TaskIndex, static_cast<int32>(INDEX_NONE));
			TestTrue(TEXT("Door opened"), Fixture->Door->LastState == TEXT("Unlocked") && !Fixture->Door->bLastPreview);
		});

		It(TEXT("should stay safe when the task list is edited after a reset"), [this]()
		{
			if (!Fixture->IsValid())
			{
				return;
			}
			// A designer removes the last task in the Details panel during Play.
			Fixture->Manager->Tasks.Pop();
			TestFalse(TEXT("Interaction with the removed task's actor is ignored"), Fixture->Manager->HandleInteraction(Fixture->Interact[2], nullptr));
			TestFalse(TEXT("Scripted completion of the removed task is refused"), Fixture->Manager->TryCompleteTask(2, nullptr));
			TestTrue(TEXT("Remaining tasks still work"), Fixture->Manager->HandleInteraction(Fixture->Interact[0], nullptr));
			Fixture->Manager->ResetPuzzle();
			TestEqual(TEXT("Reset matches the new list"), Fixture->Manager->GetTaskCount(), 2);
		});

		It(TEXT("should allow scripted completion only for available tasks"), [this]()
		{
			if (!Fixture->IsValid())
			{
				return;
			}
			TestFalse(TEXT("Locked task refused"), Fixture->Manager->TryCompleteTask(1, nullptr));
			TestFalse(TEXT("Out of range refused"), Fixture->Manager->TryCompleteTask(TaskCount, nullptr));
			TestTrue(TEXT("Available task accepted"), Fixture->Manager->TryCompleteTask(0, nullptr));
			TestEqual(TEXT("Refusal ran no On Fail"), Fixture->Fail[1]->ExecuteCount, 0);
		});

		It(TEXT("should put everything back to its On Begin state on reset"), [this]()
		{
			if (!Fixture->IsValid())
			{
				return;
			}
			for (int32 Index = 0; Index < TaskCount; ++Index)
			{
				Fixture->Manager->HandleInteraction(Fixture->Interact[Index], nullptr);
			}
			Fixture->Manager->ResetPuzzle();
			TestFalse(TEXT("No longer solved"), Fixture->Manager->IsSolved());
			TestEqual(TEXT("Nothing completed"), Fixture->Manager->GetCompletedTaskCount(), 0);
			for (int32 Index = 0; Index < TaskCount; ++Index)
			{
				TestTrue(TEXT("Before shown again"), IsShown(Fixture->Before[Index]));
				TestFalse(TEXT("After hidden again"), IsShown(Fixture->After[Index]));
				TestEqual(TEXT("On Begin ran again"), Fixture->Begin[Index]->ExecuteCount, 2);
			}
			TestTrue(TEXT("Door locked again"), Fixture->Door->LastState == TEXT("Locked"));
		});
	});

	Describe(TEXT("Requires"), [this]()
	{
		It(TEXT("should open every task when none has requirements"), [this]()
		{
			Fixture = MakeUnique<FAtlantisPuzzleFixture>(false);
			if (!TestTrue(TEXT("Fixture built"), Fixture->IsValid()))
			{
				return;
			}
			for (int32 Index = 0; Index < TaskCount; ++Index)
			{
				TestTrue(TEXT("Available"), Fixture->Manager->GetTaskState(Index) == EAtlantisPuzzleTaskState::Available);
			}
			for (const int32 Index : {2, 0, 1})
			{
				TestTrue(TEXT("Completed in any order"), Fixture->Manager->HandleInteraction(Fixture->Interact[Index], nullptr));
			}
			TestTrue(TEXT("Solved"), Fixture->Manager->IsSolved());
		});

		It(TEXT("should wait for every named requirement"), [this]()
		{
			Fixture = MakeUnique<FAtlantisPuzzleFixture>(false);
			if (!TestTrue(TEXT("Fixture built"), Fixture->IsValid()))
			{
				return;
			}
			// Task2 is the gate: it needs both Task0 and Task1 (Flooded Maintenance Station).
			Fixture->Manager->Tasks[2].Requires = {TEXT("Task0"), TEXT("Task1")};
			Fixture->Manager->ResetPuzzle();
			TestTrue(TEXT("Gate locked"), Fixture->Manager->GetTaskState(2) == EAtlantisPuzzleTaskState::Locked);
			Fixture->Manager->HandleInteraction(Fixture->Interact[1], nullptr);
			TestTrue(TEXT("Gate still locked with one done"), Fixture->Manager->GetTaskState(2) == EAtlantisPuzzleTaskState::Locked);
			Fixture->Manager->HandleInteraction(Fixture->Interact[0], nullptr);
			TestTrue(TEXT("Gate opens when both are done"), Fixture->Manager->GetTaskState(2) == EAtlantisPuzzleTaskState::Available);
		});

		It(TEXT("should follow Requires regardless of list order"), [this]()
		{
			Fixture = MakeUnique<FAtlantisPuzzleFixture>(false);
			if (!TestTrue(TEXT("Fixture built"), Fixture->IsValid()))
			{
				return;
			}
			// The chain runs Task2 -> Task0 -> Task1, the reverse of how the list is written.
			Fixture->Manager->Tasks[0].Requires = {TEXT("Task2")};
			Fixture->Manager->Tasks[1].Requires = {TEXT("Task0")};
			Fixture->Manager->ResetPuzzle();
			TestTrue(TEXT("Only Task2 open at start"), Fixture->Manager->GetTaskState(2) == EAtlantisPuzzleTaskState::Available
				&& Fixture->Manager->GetTaskState(0) == EAtlantisPuzzleTaskState::Locked
				&& Fixture->Manager->GetTaskState(1) == EAtlantisPuzzleTaskState::Locked);
			Fixture->Manager->HandleInteraction(Fixture->Interact[2], nullptr);
			TestTrue(TEXT("Task0 opens after Task2"), Fixture->Manager->GetTaskState(0) == EAtlantisPuzzleTaskState::Available);
			TestTrue(TEXT("Task1 still locked"), Fixture->Manager->GetTaskState(1) == EAtlantisPuzzleTaskState::Locked);
			Fixture->Manager->HandleInteraction(Fixture->Interact[0], nullptr);
			Fixture->Manager->HandleInteraction(Fixture->Interact[1], nullptr);
			TestTrue(TEXT("Solved in Requires order"), Fixture->Manager->IsSolved());
		});

		It(TEXT("should list the tasks for the Requires dropdown"), [this]()
		{
			Fixture = MakeUnique<FAtlantisPuzzleFixture>(false);
			if (!TestTrue(TEXT("Fixture built"), Fixture->IsValid()))
			{
				return;
			}
			TestTrue(TEXT("Every task listed"), Fixture->Manager->GetTaskNames() == TArray<FName>{TEXT("Task0"), TEXT("Task1"), TEXT("Task2")});
		});
	});

	Describe(TEXT("Validation"), [this]()
	{
		BeforeEach([this]()
		{
			Fixture = MakeUnique<FAtlantisPuzzleFixture>(true);
			TestTrue(TEXT("Fixture built"), Fixture->IsValid());
		});

		It(TEXT("should find no problems in a clean puzzle"), [this]()
		{
			if (Fixture->IsValid())
			{
				TestEqual(TEXT("No problems"), Fixture->Manager->ValidatePuzzle(), 0);
			}
		});

		It(TEXT("should warn when two tasks share an Interact With"), [this]()
		{
			if (!Fixture->IsValid())
			{
				return;
			}
			AddExpectedMessage(TEXT("both use"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
			Fixture->Manager->Tasks[2].InteractWith = Fixture->Interact[0];
			TestEqual(TEXT("One problem"), Fixture->Manager->ValidatePuzzle(), 1);
		});

		It(TEXT("should warn when two On Begin lists set the same actor"), [this]()
		{
			if (!Fixture->IsValid())
			{
				return;
			}
			AddExpectedMessage(TEXT("both set"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
			Fixture->Manager->Tasks[1].OnBegin.Add(Visible(Fixture->Manager, {Fixture->Before[0]}, true));
			TestEqual(TEXT("One problem"), Fixture->Manager->ValidatePuzzle(), 1);
		});

		It(TEXT("should warn when Set Object State picks a state the target doesn't have"), [this]()
		{
			if (!Fixture->IsValid())
			{
				return;
			}
			AddExpectedMessage(TEXT("not one of its states"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
			Fixture->Manager->OnSolved.Add(State(Fixture->Manager, Fixture->Door, TEXT("Membrane")));
			TestEqual(TEXT("One problem"), Fixture->Manager->ValidatePuzzle(), 1);
		});

		It(TEXT("should warn when Set Object State targets an actor that isn't a puzzle object, or has no state"), [this]()
		{
			if (!Fixture->IsValid())
			{
				return;
			}
			AddExpectedMessage(TEXT("does not implement Atlantis Puzzle Object"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
			AddExpectedMessage(TEXT("has no State picked"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
			Fixture->Manager->OnSolved.Add(State(Fixture->Manager, Fixture->Unrelated, TEXT("Unlocked")));
			Fixture->Manager->OnSolved.Add(State(Fixture->Manager, Fixture->Door, NAME_None));
			TestEqual(TEXT("Two problems"), Fixture->Manager->ValidatePuzzle(), 2);
		});

		It(TEXT("should warn when two tasks share a name"), [this]()
		{
			if (!Fixture->IsValid())
			{
				return;
			}
			AddExpectedMessage(TEXT("same Task Name"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
			Fixture->Manager->Tasks[2].TaskName = TEXT("Task0");
			TestEqual(TEXT("One problem"), Fixture->Manager->ValidatePuzzle(), 1);
		});

		It(TEXT("should not warn about a Requires entry that has not been picked yet"), [this]()
		{
			if (!Fixture->IsValid())
			{
				return;
			}
			Fixture->Manager->Tasks[1].Requires.Add(NAME_None);
			TestEqual(TEXT("No problems"), Fixture->Manager->ValidatePuzzle(), 0);
		});

		It(TEXT("should warn about requirements that name no other task"), [this]()
		{
			if (!Fixture->IsValid())
			{
				return;
			}
			AddExpectedMessage(TEXT("not another task"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 2);
			Fixture->Manager->Tasks[1].Requires = {TEXT("Missing"), TEXT("Task1")};
			TestEqual(TEXT("Two problems"), Fixture->Manager->ValidatePuzzle(), 2);
		});
	});

	Describe(TEXT("Reset"), [this]()
	{
		BeforeEach([this]()
		{
			Fixture = MakeUnique<FAtlantisPuzzleFixture>(true);
			TestTrue(TEXT("Fixture built"), Fixture->IsValid());
		});

		It(TEXT("should run a reset requested from On Solved once the solve finishes"), [this]()
		{
			if (!Fixture->IsValid())
			{
				return;
			}
			UAtlantisPuzzleTestResetAction* Reset = NewObject<UAtlantisPuzzleTestResetAction>(Fixture->Manager);
			Fixture->Manager->OnSolved.Add(Reset);
			for (int32 Index = 0; Index < TaskCount; ++Index)
			{
				Fixture->Manager->HandleInteraction(Fixture->Interact[Index], nullptr);
			}
			TestEqual(TEXT("Solved once"), Fixture->Listener->SolvedCount, 1);
			TestFalse(TEXT("Ready to play again"), Fixture->Manager->IsSolved());
			TestTrue(TEXT("Looks back at start"), IsShown(Fixture->Before[0]));
		});

		It(TEXT("should ignore a reset requested while resetting instead of looping"), [this]()
		{
			if (!Fixture->IsValid())
			{
				return;
			}
			AddExpectedMessage(TEXT("already resetting"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
			UAtlantisPuzzleTestResetAction* Reset = NewObject<UAtlantisPuzzleTestResetAction>(Fixture->Manager);
			Fixture->Manager->Tasks[0].OnBegin.Add(Reset);
			Fixture->Manager->ResetPuzzle();
			TestEqual(TEXT("On Begin ran once, no loop"), Reset->ExecuteCount, 1);
		});

		It(TEXT("Music Room: a wrong attempt resets the sequence when Reset On Fail is on"), [this]()
		{
			if (!Fixture->IsValid())
			{
				return;
			}
			Fixture->Manager->bResetOnFail = true;
			Fixture->Manager->HandleInteraction(Fixture->Interact[0], nullptr);
			Fixture->Manager->HandleInteraction(Fixture->Interact[2], nullptr);
			TestEqual(TEXT("On Fail ran"), Fixture->Fail[2]->ExecuteCount, 1);
			TestEqual(TEXT("Progress cleared"), Fixture->Manager->GetCompletedTaskCount(), 0);
			TestTrue(TEXT("First task looks back at start"), IsShown(Fixture->Before[0]) && !IsShown(Fixture->After[0]));
		});
	});

	Describe(TEXT("BuiltInActions"), [this]()
	{
		BeforeEach([this]()
		{
			Fixture = MakeUnique<FAtlantisPuzzleFixture>(true);
			TestTrue(TEXT("Fixture built"), Fixture->IsValid());
		});

		It(TEXT("Complete Puzzle Task should do nothing without a Task Name"), [this]()
		{
			if (!Fixture->IsValid())
			{
				return;
			}
			AddExpectedMessage(TEXT("set both Puzzle and Task Name"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
			Fixture->Manager->Tasks[0].TaskName = NAME_None;
			UAtlantisPuzzleAction_CompletePuzzleTask* Chain = NewObject<UAtlantisPuzzleAction_CompletePuzzleTask>(Fixture->Manager);
			Chain->Puzzle = Fixture->Manager;
			FAtlantisPuzzleActionContext Context;
			Context.Manager = Fixture->Manager;
			Chain->Execute(Context);
			TestTrue(TEXT("Unnamed task untouched"), Fixture->Manager->GetTaskState(0) == EAtlantisPuzzleTaskState::Available);
		});

		It(TEXT("Complete Puzzle Task should chain one puzzle into another (Drowned Pumpworks)"), [this]()
		{
			if (!Fixture->IsValid())
			{
				return;
			}
			AAtlantisPuzzleManager* LowerWorks = Fixture->World->SpawnActor<AAtlantisPuzzleManager>();
			AActor* Debris = Fixture->World->SpawnActor<AActor>();
			AddTask(LowerWorks, TEXT("ClearDebris"), Debris);
			UAtlantisPuzzleAction_CompletePuzzleTask* Chain = NewObject<UAtlantisPuzzleAction_CompletePuzzleTask>(LowerWorks);
			Chain->Puzzle = Fixture->Manager;
			Chain->TaskName = TEXT("Task0");
			LowerWorks->OnSolved.Add(Chain);
			LowerWorks->ResetPuzzle();

			TestTrue(TEXT("Debris routes to the lower works"), UAtlantisPuzzleLibrary::NotifyPuzzleInteraction(Debris, nullptr));
			TestTrue(TEXT("Lower works solved"), LowerWorks->IsSolved());
			TestTrue(TEXT("Main puzzle's task completed by the chain"), Fixture->Manager->GetTaskState(0) == EAtlantisPuzzleTaskState::Completed);
		});

		It(TEXT("Play Sound should fall back from Play At to the task actor to the manager"), [this]()
		{
			if (!Fixture->IsValid())
			{
				return;
			}
			UAtlantisPuzzleAction_PlaySound* Sound = NewObject<UAtlantisPuzzleAction_PlaySound>(Fixture->Manager);
			FAtlantisPuzzleActionContext Context;
			Context.Manager = Fixture->Manager;
			TestTrue(TEXT("Puzzle-level lists play at the manager"), Sound->GetSoundLocation(Context) == Fixture->Manager);
			Context.TaskActor = Fixture->Interact[0];
			TestTrue(TEXT("A task plays at its actor"), Sound->GetSoundLocation(Context) == Fixture->Interact[0]);
			Sound->PlayAt = Fixture->Unrelated;
			TestTrue(TEXT("Play At wins"), Sound->GetSoundLocation(Context) == Fixture->Unrelated);
		});

		It(TEXT("Set Actors Visible should restore original collision when showing"), [this]()
		{
			if (!Fixture->IsValid())
			{
				return;
			}
			AActor* NoCollision = Fixture->World->SpawnActor<AActor>();
			NoCollision->SetActorEnableCollision(false);
			Fixture->Manager->Tasks[0].OnBegin.Add(Visible(Fixture->Manager, {NoCollision}, false));
			Fixture->Manager->Tasks[0].OnComplete.Add(Visible(Fixture->Manager, {NoCollision}, true));
			Fixture->Manager->ResetPuzzle();
			Fixture->Manager->HandleInteraction(Fixture->Interact[0], nullptr);
			TestFalse(TEXT("Shown"), NoCollision->IsHidden());
			TestFalse(TEXT("Still no collision"), NoCollision->GetActorEnableCollision());
		});

		It(TEXT("Set Actors Visible should hide attached actors with their parent unless told not to"), [this]()
		{
			if (!Fixture->IsValid())
			{
				return;
			}
			// A power box with a cable segment attached to it, as in L_Puzzle_Test_01.
			AActor* Box = Fixture->World->SpawnActor<AActor>();
			AActor* Cable = Fixture->World->SpawnActor<AActor>();
			Box->SetRootComponent(NewObject<USceneComponent>(Box));
			Cable->SetRootComponent(NewObject<USceneComponent>(Cable));
			Cable->AttachToActor(Box, FAttachmentTransformRules::KeepWorldTransform);

			UAtlantisPuzzleAction_SetActorsVisible* Hide = Visible(Fixture->Manager, {Box}, false);
			FAtlantisPuzzleActionContext Context;
			Context.Manager = Fixture->Manager;
			Hide->Execute(Context);
			TestTrue(TEXT("Parent hidden"), Box->IsHidden());
			TestTrue(TEXT("Attached child hidden too"), Cable->IsHidden() && !Cable->GetActorEnableCollision());

			Visible(Fixture->Manager, {Box}, true)->Execute(Context);
			TestTrue(TEXT("Both shown again"), IsShown(Box) && IsShown(Cable));

			Hide->bIncludeAttachedActors = false;
			Hide->Execute(Context);
			TestTrue(TEXT("Only the parent hidden when unticked"), Box->IsHidden() && IsShown(Cable));
		});

		It(TEXT("Set Object State should offer the states its targets declare"), [this]()
		{
			if (!Fixture->IsValid())
			{
				return;
			}
			UAtlantisPuzzleAction_SetObjectState* Action = State(Fixture->Manager, Fixture->Door, NAME_None);
			TestTrue(TEXT("Dropdown lists the door's states, default first"), Action->GetStateOptions() == TArray<FName>{TEXT("Locked"), TEXT("Unlocked")});
			Action->Targets.Add(Fixture->Unrelated);
			TestEqual(TEXT("A target with no states leaves nothing in common"), Action->GetStateOptions().Num(), 0);
		});

		It(TEXT("Library should prefer a puzzle that can complete the task"), [this]()
		{
			if (!Fixture->IsValid())
			{
				return;
			}
			// A second puzzle whose task on Interact[1] is open, while the fixture's is locked.
			AAtlantisPuzzleManager* Other = Fixture->World->SpawnActor<AAtlantisPuzzleManager>();
			AddTask(Other, TEXT("Shared"), Fixture->Interact[1]);
			Other->ResetPuzzle();
			TestTrue(TEXT("Routed to the open task"), UAtlantisPuzzleLibrary::FindPuzzleManagerForActor(Fixture->Interact[1]) == Other);
			TestNull(TEXT("No puzzle for an unrelated actor"), UAtlantisPuzzleLibrary::FindPuzzleManagerForActor(Fixture->Unrelated));
		});
	});

	Describe(TEXT("FloodedStation"), [this]()
	{
		It(TEXT("FDS-012 Flooded Station: power off, repair, power on, button"), [this]()
		{
			FEditorScriptExecutionGuard ScriptGuard;
			TStrongObjectPtr<UWorld> World(UWorld::CreateWorld(EWorldType::Game, false));
			if (!TestNotNull(TEXT("World"), World.Get()))
			{
				return;
			}
			AAtlantisPuzzleManager* M = World->SpawnActor<AAtlantisPuzzleManager>();
			AActor* LeverOn = World->SpawnActor<AActor>();
			AActor* LeverOff = World->SpawnActor<AActor>();
			AActor* Broken = World->SpawnActor<AActor>();
			AActor* Fixed = World->SpawnActor<AActor>();
			AActor* Discharge = World->SpawnActor<AActor>();
			AActor* Button = World->SpawnActor<AActor>();
			AActor* Lit = World->SpawnActor<AActor>();
			AAtlantisPuzzleTestDoor* Door = World->SpawnActor<AAtlantisPuzzleTestDoor>();
			if (!TestNotNull(TEXT("Manager"), M) || !Door)
			{
				World->DestroyWorld(false);
				return;
			}

			M->Tasks.Reserve(4);
			FAtlantisPuzzleTask& PowerOff = AddTask(M, TEXT("PowerOff"), LeverOn);
			PowerOff.OnBegin = {Visible(M, {LeverOn, Discharge}, true), Visible(M, {LeverOff}, false)};
			PowerOff.OnComplete = {Visible(M, {LeverOn, Discharge}, false), Visible(M, {LeverOff}, true)};
			FAtlantisPuzzleTask& Repair = AddTask(M, TEXT("RepairLine"), Broken, {TEXT("PowerOff")});
			Repair.OnBegin = {Visible(M, {Broken}, true), Visible(M, {Fixed}, false)};
			Repair.OnComplete = {Visible(M, {Broken}, false), Visible(M, {Fixed}, true)};
			FAtlantisPuzzleTask& PowerOn = AddTask(M, TEXT("PowerOn"), LeverOff, {TEXT("RepairLine")});
			PowerOn.OnComplete = {Visible(M, {LeverOff}, false), Visible(M, {LeverOn, Lit}, true)};
			FAtlantisPuzzleTask& Press = AddTask(M, TEXT("PressButton"), Button, {TEXT("PowerOn")});
			Press.OnBegin = {Visible(M, {Lit}, false)};
			M->OnBegin = {State(M, Door, TEXT("Locked"))};
			M->OnSolved = {State(M, Door, TEXT("Unlocked"))};
			TestEqual(TEXT("Clean setup"), M->ValidatePuzzle(), 0);
			M->ResetPuzzle();

			TestTrue(TEXT("Start: lever on, line discharging, door locked"), IsShown(LeverOn) && !IsShown(LeverOff) && IsShown(Discharge) && Door->LastState == TEXT("Locked"));
			M->HandleInteraction(Broken, nullptr);
			TestTrue(TEXT("Live line can't be repaired"), M->GetTaskState(1) == EAtlantisPuzzleTaskState::Locked && IsShown(Broken));
			M->HandleInteraction(LeverOn, nullptr);
			TestTrue(TEXT("Power off"), !IsShown(LeverOn) && IsShown(LeverOff) && !IsShown(Discharge));
			M->HandleInteraction(Broken, nullptr);
			TestTrue(TEXT("Line repaired"), IsShown(Fixed) && !IsShown(Broken));
			M->HandleInteraction(LeverOff, nullptr);
			TestTrue(TEXT("Power on, button lit"), IsShown(LeverOn) && !IsShown(LeverOff) && IsShown(Lit));
			TestFalse(TEXT("Discharge stays off"), IsShown(Discharge));
			M->HandleInteraction(Button, nullptr);
			TestTrue(TEXT("Door opens"), M->IsSolved() && Door->LastState == TEXT("Unlocked"));
			World->DestroyWorld(false);
		});
	});

#if WITH_EDITOR
	Describe(TEXT("EditorPreview"), [this]()
	{
		It(TEXT("should show one task's start or completed look only, and skip one-shot actions"), [this]()
		{
			FEditorScriptExecutionGuard ScriptGuard;
			TStrongObjectPtr<UWorld> World(UWorld::CreateWorld(EWorldType::Editor, false));
			if (!TestNotNull(TEXT("World"), World.Get()))
			{
				return;
			}
			AAtlantisPuzzleManager* M = World->SpawnActor<AAtlantisPuzzleManager>();
			AActor* A = World->SpawnActor<AActor>();
			AActor* B = World->SpawnActor<AActor>();
			AActor* Other = World->SpawnActor<AActor>();
			AAtlantisPuzzleTestDoor* Door = World->SpawnActor<AAtlantisPuzzleTestDoor>();
			if (!TestNotNull(TEXT("Manager"), M) || !Door)
			{
				World->DestroyWorld(false);
				return;
			}
			M->Tasks.Reserve(2);
			UAtlantisPuzzleTestAction* OneShot = NewObject<UAtlantisPuzzleTestAction>(M);
			FAtlantisPuzzleTask& Task = AddTask(M, TEXT("Swap"), A);
			Task.OnBegin = {Visible(M, {A}, true), Visible(M, {B}, false)};
			Task.OnComplete = {Visible(M, {A}, false), Visible(M, {B}, true), OneShot};
			FAtlantisPuzzleTask& Second = AddTask(M, TEXT("Second"), Other);
			Second.OnBegin = {Visible(M, {Other}, false)};
			M->OnSolved = {State(M, Door, TEXT("Unlocked"))};

			M->PreviewTask = TEXT("Swap");
			M->ShowTaskCompleted();
			TestTrue(TEXT("A hidden in editor"), A->IsTemporarilyHiddenInEditor());
			TestFalse(TEXT("B shown in editor"), B->IsTemporarilyHiddenInEditor());
			TestFalse(TEXT("Saved game visibility untouched"), A->IsHidden());
			TestEqual(TEXT("One-shot action skipped in preview"), OneShot->ExecuteCount, 0);
			TestFalse(TEXT("Other task's look not applied"), Other->IsTemporarilyHiddenInEditor());
			TestTrue(TEXT("Preview does not start the puzzle"), M->GetTaskState(0) == EAtlantisPuzzleTaskState::Locked);

			M->ShowTaskStart();
			TestFalse(TEXT("A shown again"), A->IsTemporarilyHiddenInEditor());
			TestTrue(TEXT("B hidden"), B->IsTemporarilyHiddenInEditor());

			TestTrue(TEXT("Dropdown lists every task"), M->GetTaskNames() == TArray<FName>{TEXT("Swap"), TEXT("Second")});

			M->ShowSolved();
			TestTrue(TEXT("Door previewed open"), Door->LastState == TEXT("Unlocked") && Door->bLastPreview);

			UAtlantisPuzzleTestPreviewAction* Custom = NewObject<UAtlantisPuzzleTestPreviewAction>(M);
			M->Tasks[0].OnComplete.Add(Custom);
			M->ShowTaskCompleted();
			TestEqual(TEXT("Custom look action runs in the preview"), Custom->PreviewCount, 1);

			M->ClearPreview();
			TestFalse(TEXT("Clear unhides B"), B->IsTemporarilyHiddenInEditor());
			TestEqual(TEXT("Clear Preview reaches custom actions"), Custom->ClearCount, 1);
			TestTrue(TEXT("Clear returns the door to its default (first) state"), Door->LastState == TEXT("Locked"));
			World->DestroyWorld(false);
		});
	});
#endif
}

#endif
