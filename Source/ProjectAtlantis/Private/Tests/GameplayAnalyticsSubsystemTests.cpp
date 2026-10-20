// Copyright (c) 2026 ASGC
#if WITH_DEV_AUTOMATION_TESTS
#include "Analytics/GameplayAnalyticsSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/Guid.h"
#include "UObject/Class.h"
#include "UObject/StrongObjectPtr.h"

struct FGameplayAnalyticsTestAccess final
{
public:
	static bool Run(FAutomationTestBase& Test, const FString& TestCase)
	{
		FGameplayAnalyticsTestAccess Fixture;
		if (TestCase == TEXT("Aggregation"))
		{
			return Fixture.TestAggregation(Test);
		}
		if (TestCase == TEXT("BudgetAndFairness"))
		{
			return Fixture.TestBudgetAndFairness(Test);
		}
		if (TestCase == TEXT("FailureAndReentrancy"))
		{
			return Fixture.TestFailureAndReentrancy(Test);
		}
		if (TestCase == TEXT("RolloverAndSessions"))
		{
			return Fixture.TestRolloverAndSessions(Test);
		}
		if (TestCase == TEXT("Persistence"))
		{
			return Fixture.TestPersistence(Test);
		}
		if (TestCase == TEXT("WorldLifecycle"))
		{
			return Fixture.TestWorldLifecycle(Test);
		}
		if (TestCase == TEXT("BoundsAndBlueprint"))
		{
			return Fixture.TestBoundsAndBlueprint(Test);
		}
		if (TestCase == TEXT("AsyncOrderingAndSnapshots"))
		{
			return Fixture.TestAsyncOrderingAndSnapshots(Test);
		}
		if (TestCase == TEXT("AsyncDayRollover"))
		{
			return Fixture.TestAsyncDayRollover(Test);
		}
		if (TestCase == TEXT("AsyncEightHourCutoff"))
		{
			return Fixture.TestAsyncEightHourCutoff(Test);
		}
		if (TestCase == TEXT("AsyncFailureAndRetry"))
		{
			return Fixture.TestAsyncFailureAndRetry(Test);
		}
		if (TestCase == TEXT("AsyncShutdownOrdering"))
		{
			return Fixture.TestAsyncShutdownOrdering(Test);
		}
		if (TestCase == TEXT("AsyncSpacingBudget"))
		{
			return Fixture.TestAsyncSpacingBudget(Test);
		}
		Test.AddError(FString::Printf(TEXT("Unknown analytics test case: %s"), *TestCase));
		return false;
	}

private:
	FGameplayAnalyticsTestAccess()
		: GameInstance(NewObject<UGameInstance>(GEngine))
		, AnalyticsSubsystem(CreateManager(GameInstance.Get()))
	{
		AnalyticsSubsystem->TestSink = [this](const FString& EventId, double Value)
		{
			SubmittedValues.Add(EventId, Value);
			++SubmissionCount;
			return true;
		};
	}

	static UGameplayAnalyticsSubsystem* CreateManager(UGameInstance* OwningGameInstance)
	{
		UGameplayAnalyticsSubsystem* const CreatedManager = NewObject<UGameplayAnalyticsSubsystem>(OwningGameInstance);
		CreatedManager->State = NewObject<UGameplayAnalyticsSaveGame>(CreatedManager);
		CreatedManager->TestUtcDay = TEXT("2026-10-17");
		CreatedManager->TestFrame = 0;
		CreatedManager->RefreshUtcDay();
		CreatedManager->TestPersist = []
		{
			return true;
		};
		return CreatedManager;
	}

	static void PumpPersistenceFrame(UGameplayAnalyticsSubsystem* Manager, float DeltaSeconds = 0.0f)
	{
		const uint64 Frame = Manager->TestFrame.IsSet() ? Manager->TestFrame.GetValue() : 0;
		Manager->TestFrame = Frame + 1;
		Manager->TickPersistence(DeltaSeconds);
	}

	void DrainPersistence()
	{
		for (int32 Step = 0; Step < 4096; ++Step)
		{
			if (AnalyticsSubsystem->SaveRetrySeconds > 0.0)
			{
				return;
			}
			if (AnalyticsSubsystem->SaveFuture.IsValid())
			{
				// Waiting is test-only; production polls the worker without blocking gameplay.
				AnalyticsSubsystem->SaveFuture.Wait();
			}
			else if (!AnalyticsSubsystem->bFlushRequested && !AnalyticsSubsystem->bSaveRequested)
			{
				return;
			}
			PumpPersistenceFrame(AnalyticsSubsystem);
		}
		checkNoEntry();
	}

	bool TestAggregation(FAutomationTestBase& Test)
	{
		AnalyticsSubsystem->RegisterMetric(TEXT("Test:Average"), EGameplayAnalyticsAggregation::Average);
		AnalyticsSubsystem->RegisterMetric(TEXT("Test:Latest"), EGameplayAnalyticsAggregation::Latest);
		Test.TestFalse(TEXT("Cannot reinterpret existing key"),
			AnalyticsSubsystem->RegisterMetric(TEXT("Test:Average"), EGameplayAnalyticsAggregation::Sum));
		Test.TestFalse(TEXT("Reserved summary ID"),
			AnalyticsSubsystem->RegisterMetric(TEXT("Session:GameplayDuration"), EGameplayAnalyticsAggregation::Count));
		Test.TestFalse(
			TEXT("Empty hierarchy segment"), AnalyticsSubsystem->RegisterMetric(TEXT("Test::Empty"), EGameplayAnalyticsAggregation::Count));
		for (const float Value : {2.f, 4.f, 12.f})
		{
			AnalyticsSubsystem->RecordEvent(TEXT("Test:Count"));
			AnalyticsSubsystem->RecordValue(TEXT("Test:Sum"), Value);
			AnalyticsSubsystem->RecordValue(TEXT("Test:Average"), Value);
			AnalyticsSubsystem->RecordValue(TEXT("Test:Latest"), Value);
		}
		AnalyticsSubsystem->RecordPerformanceValue(TEXT("Performance:MapLoad:Test"), 10.f);
		AnalyticsSubsystem->RecordPerformanceValue(TEXT("Performance:MapLoad:Test"), 30.f);
		Test.TestEqual(TEXT("Recording does not submit"), SubmissionCount, 0);
		AnalyticsSubsystem->FlushAnalytics();
		DrainPersistence();
		Test.TestEqual(TEXT("Early manual flush earns nothing"), SubmissionCount, 0);
		AnalyticsSubsystem->AdvanceGameplayTime(299);
		DrainPersistence();
		Test.TestEqual(TEXT("No early scheduled send"), SubmissionCount, 0);
		AnalyticsSubsystem->AdvanceGameplayTime(1);
		DrainPersistence();
		Test.TestEqual(TEXT("Count"), SubmittedValues.FindRef(TEXT("Test:Count")), 3.0);
		Test.TestEqual(TEXT("Sum"), SubmittedValues.FindRef(TEXT("Test:Sum")), 18.0);
		Test.TestEqual(TEXT("Average is sample-weighted across pending intervals"), SubmittedValues.FindRef(TEXT("Test:Average")), 6.0);
		Test.TestEqual(TEXT("Latest"), SubmittedValues.FindRef(TEXT("Test:Latest")), 12.0);
		Test.TestEqual(TEXT("Performance mean milliseconds"), SubmittedValues.FindRef(TEXT("Performance:MapLoad:Test")), 20.0);
		AnalyticsSubsystem->FlushAnalytics();
		DrainPersistence();
		Test.TestEqual(TEXT("Unchanged metrics not resent"), SubmissionCount, 5);
		return true;
	}

	bool TestBudgetAndFairness(FAutomationTestBase& Test)
	{
		for (int32 Index = 0; Index < 100; ++Index)
		{
			AnalyticsSubsystem->RegisterMetric(
				FString::Printf(TEXT("Test:M%d"), Index), EGameplayAnalyticsAggregation::Count, Index == 99 ? -100 : 100);
		}
		for (int32 Interval = 0; Interval < 96; ++Interval)
		{
			for (int32 Index = 0; Index < 100; ++Index)
			{
				AnalyticsSubsystem->RecordEvent(FString::Printf(TEXT("Test:M%d"), Index));
			}
			AnalyticsSubsystem->AdvanceGameplayTime(300);
			AnalyticsSubsystem->FlushAnalytics();
			AnalyticsSubsystem->EndGameplaySession(); // repeated short sessions cannot replenish periodic allowance
			DrainPersistence();
			if (Interval == 47)
			{
				Test.TestEqual(TEXT("Four-hour phase cap"), AnalyticsSubsystem->State->PeriodicSubmitted, 320);
			}
		}
		Test.TestEqual(TEXT("Eight-hour phase cap"), AnalyticsSubsystem->State->PeriodicSubmitted, 470);
		Test.TestEqual(TEXT("Separate summary reserve"), AnalyticsSubsystem->State->SummarySubmitted, 10);
		Test.TestTrue(TEXT("Aging eventually serves lowest priority"), SubmittedValues.Contains(TEXT("Test:M99")));
		Test.TestTrue(TEXT("Combined day cap"), SubmissionCount <= 480);
		AnalyticsSubsystem->AdvanceGameplayTime(300);
		DrainPersistence();
		AnalyticsSubsystem->RecordEvent(TEXT("Test:AfterEight"));
		const int32 PreviousSubmissionCount = SubmissionCount;
		AnalyticsSubsystem->FlushAnalytics();
		DrainPersistence();
		Test.TestEqual(TEXT("No periodic submission after eight hours"), SubmissionCount, PreviousSubmissionCount);
		Test.TestEqual(
			TEXT("Post-cap observation retained"), AnalyticsSubsystem->State->Metrics.FindChecked(TEXT("Test:AfterEight")).Count, int64(1));
		return true;
	}

	bool TestFailureAndReentrancy(FAutomationTestBase& Test)
	{
		Test.AddExpectedMessagePlain(TEXT("Analytics checkpoint failed; retry deferred for one minute"), ELogVerbosity::Warning,
			EAutomationExpectedMessageFlags::Contains, 2);
		AnalyticsSubsystem->bTestProviderUnavailable = true;
		AnalyticsSubsystem->RecordValue(TEXT("Test:Value"), 5.f);
		AnalyticsSubsystem->AdvanceGameplayTime(300);
		DrainPersistence();
		Test.TestEqual(TEXT("Unavailable provider retains pending"),
			AnalyticsSubsystem->State->Metrics.FindChecked(TEXT("Test:Value")).Count, int64(1));
		AnalyticsSubsystem->bTestProviderUnavailable = false;
		AnalyticsSubsystem->TestSink = [this](const FString&, double)
		{
			++SubmissionCount;
			return false;
		};
		AnalyticsSubsystem->FlushAnalytics();
		DrainPersistence();
		Test.TestEqual(TEXT("Failed enqueue keeps pending"), AnalyticsSubsystem->State->Metrics.FindChecked(TEXT("Test:Value")).Sum, 5.0);
		Test.TestEqual(TEXT("Failed enqueue restores quota"), AnalyticsSubsystem->State->PeriodicSubmitted, 0);
		AnalyticsSubsystem->TestPersist = []
		{
			return false;
		};
		const int32 PreviousSubmissionCount = SubmissionCount;
		AnalyticsSubsystem->FlushAnalytics();
		DrainPersistence();
		Test.TestEqual(TEXT("Disk failure blocks SDK call"), SubmissionCount, PreviousSubmissionCount);
		int32 FailedCheckpoints = 0;
		AnalyticsSubsystem->CheckpointSeconds = 0.0;
		AnalyticsSubsystem->TestPersist = [&FailedCheckpoints]
		{
			++FailedCheckpoints;
			return false;
		};
		AnalyticsSubsystem->AdvanceGameplayTime(60);
		PumpPersistenceFrame(AnalyticsSubsystem, 60.0f);
		DrainPersistence();
		AnalyticsSubsystem->AdvanceGameplayTime(1);
		DrainPersistence();
		Test.TestEqual(TEXT("Disk failure does not retry every frame"), FailedCheckpoints, 1);
		AnalyticsSubsystem->TestPersist = []
		{
			return true;
		};
		PumpPersistenceFrame(AnalyticsSubsystem, 60.0f);
		DrainPersistence();
		AnalyticsSubsystem->TestSink = [this, &Test](const FString& Id, double Value)
		{
			Test.TestEqual(TEXT("Snapshot numeric value"), Value, 5.0);
			AnalyticsSubsystem->RecordValue(Id, 7.f);
			for (int32 Index = 0; Index < 100; ++Index)
			{
				AnalyticsSubsystem->RegisterMetric(FString::Printf(TEXT("Rehash:M%d"), Index), EGameplayAnalyticsAggregation::Count);
			}
			AnalyticsSubsystem->FlushAnalytics(); // recursion must not resend the snapshot
			++SubmissionCount;
			return true;
		};
		AnalyticsSubsystem->FlushAnalytics();
		DrainPersistence();
		Test.TestEqual(
			TEXT("New observation survives snapshot"), AnalyticsSubsystem->State->Metrics.FindChecked(TEXT("Test:Value")).Sum, 7.0);
		Test.TestEqual(TEXT("One reservation committed"), AnalyticsSubsystem->State->PeriodicSubmitted, 1);
		return true;
	}

	bool TestRolloverAndSessions(FAutomationTestBase& Test)
	{
		AnalyticsSubsystem->AdvanceGameplayTime(100);
		DrainPersistence();
		AnalyticsSubsystem->EndGameplaySession();
		DrainPersistence();
		AnalyticsSubsystem->EndGameplaySession();
		DrainPersistence();
		Test.TestEqual(TEXT("Duplicate lifecycle callback"), AnalyticsSubsystem->State->SummarySubmitted, 1);
		Test.TestEqual(TEXT("Duration seconds"), SubmittedValues.FindRef(TEXT("Session:GameplayDuration")), 100.0);
		AnalyticsSubsystem->AdvanceGameplayTime(200);
		DrainPersistence();
		Test.TestEqual(TEXT("Sessions share active day time"), AnalyticsSubsystem->State->ActiveGameplaySeconds, 300.0);
		AnalyticsSubsystem->State->SummarySubmitted = 10;
		AnalyticsSubsystem->EndGameplaySession();
		DrainPersistence();
		Test.TestEqual(TEXT("Excess summary deferred"), AnalyticsSubsystem->State->PendingSessionSeconds, 200.0);
		AnalyticsSubsystem->RecordValue(TEXT("Test:Pending"), 42.f);
		AnalyticsSubsystem->TestUtcDay = TEXT("2026-10-18");
		AnalyticsSubsystem->FlushAnalytics();
		DrainPersistence();
		Test.TestEqual(TEXT("UTC quota resets"), AnalyticsSubsystem->State->PeriodicSubmitted, 0);
		Test.TestEqual(TEXT("UTC active time resets"), AnalyticsSubsystem->State->ActiveGameplaySeconds, 0.0);
		Test.TestEqual(TEXT("Deferred summary retried"), AnalyticsSubsystem->State->SummarySubmitted, 1);
		Test.TestEqual(TEXT("Pending metric crosses day"), AnalyticsSubsystem->State->Metrics.FindChecked(TEXT("Test:Pending")).Sum, 42.0);
		AnalyticsSubsystem->TestUtcDay = TEXT("2026-10-17");
		AnalyticsSubsystem->RefreshUtcDay();
		Test.TestEqual(TEXT("Backwards clock does not replenish"), AnalyticsSubsystem->State->SummarySubmitted, 1);
		return true;
	}

	bool TestPersistence(FAutomationTestBase& Test)
	{
		AnalyticsSubsystem->SlotName = TEXT("AnalyticsAutomation_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
		AnalyticsSubsystem->TestPersist = nullptr;
		AnalyticsSubsystem->RecordEvent(TEXT("Test:Pending"));
		AnalyticsSubsystem->AdvanceGameplayTime(150);
		DrainPersistence();
		AnalyticsSubsystem->RequestCheckpoint();
		DrainPersistence();
		Test.TestTrue(TEXT("Real async SaveGame checkpoint"), UGameplayStatics::DoesSaveGameExist(AnalyticsSubsystem->SlotName, 0));
		UGameplayAnalyticsSubsystem* const RestoredAnalytics = CreateManager(GameInstance.Get());
		RestoredAnalytics->State = Cast<UGameplayAnalyticsSaveGame>(UGameplayStatics::LoadGameFromSlot(AnalyticsSubsystem->SlotName, 0));
		if (!Test.TestNotNull(TEXT("SaveGame restored"), RestoredAnalytics->State.Get()))
		{
			return false;
		}
		RestoredAnalytics->ValidateState();
		Test.TestEqual(
			TEXT("Pending data survives restart"), RestoredAnalytics->State->Metrics.FindChecked(TEXT("Test:Pending")).Count, int64(1));
		Test.TestEqual(TEXT("Open session survives checkpoint"), RestoredAnalytics->State->OpenSessionSeconds, 150.0);
		Test.TestEqual(TEXT("Scheduler survives restart"), RestoredAnalytics->State->IntervalSeconds, 150.0);
		AnalyticsSubsystem->AdvanceGameplayTime(150); // submits and persists reservation
		DrainPersistence();
		RestoredAnalytics->State = Cast<UGameplayAnalyticsSaveGame>(UGameplayStatics::LoadGameFromSlot(AnalyticsSubsystem->SlotName, 0));
		Test.TestEqual(TEXT("Sent snapshot not replayed after restart"),
			RestoredAnalytics->State->Metrics.FindChecked(TEXT("Test:Pending")).Count, int64(0));
		Test.TestEqual(TEXT("Quota survives restart"), RestoredAnalytics->State->PeriodicSubmitted, 1);
		UGameplayStatics::DeleteGameInSlot(AnalyticsSubsystem->SlotName, 0);
		return true;
	}

	bool TestWorldLifecycle(FAutomationTestBase& Test)
	{
		GameInstance->InitializeStandalone();
		AnalyticsSubsystem = GameInstance->GetSubsystem<UGameplayAnalyticsSubsystem>();
		if (!Test.TestNotNull(TEXT("Automatic GameInstance subsystem initialization"), AnalyticsSubsystem))
		{
			return false;
		}
		AnalyticsSubsystem->TestFrame = 0;
		AnalyticsSubsystem->State = NewObject<UGameplayAnalyticsSaveGame>(AnalyticsSubsystem);
		AnalyticsSubsystem->TestPersist = []
		{
			return true;
		};
		AnalyticsSubsystem->TestUtcDay = TEXT("2026-10-17");
		AnalyticsSubsystem->TestSink = [this](const FString& Id, double Value)
		{
			SubmittedValues.Add(Id, Value);
			++SubmissionCount;
			return true;
		};
		UWorld* const World = GameInstance->GetWorld();
		Test.TestTrue(TEXT("Primary gameplay world accepted"), AnalyticsSubsystem->IsActiveGameplayWorld(World));
		World->GetWorldSettings()->SetPauserPlayerState(World->SpawnActor<APlayerState>());
		Test.TestFalse(TEXT("Paused world excluded"), AnalyticsSubsystem->IsActiveGameplayWorld(World));
		World->GetWorldSettings()->SetPauserPlayerState(nullptr);
		Test.TestTrue(TEXT("Resumed world accepted"), AnalyticsSubsystem->IsActiveGameplayWorld(World));
		AnalyticsSubsystem->AdvanceGameplayTime(100);
		DrainPersistence();
		AnalyticsSubsystem->OnPreLoadMap(*GameInstance->GetWorldContext(), TEXT("/Game/Maps/L_SecondGameplay"));
		DrainPersistence();
		Test.TestTrue(TEXT("Gameplay transition preserves session"), AnalyticsSubsystem->bSessionOpen);
		AnalyticsSubsystem->AdvanceGameplayTime(50);
		DrainPersistence();
		AnalyticsSubsystem->OnPreLoadMap(*GameInstance->GetWorldContext(), TEXT("/Game/Maps/L_MainMenu"));
		DrainPersistence();
		AnalyticsSubsystem->OnPreLoadMap(*GameInstance->GetWorldContext(), TEXT("/Game/Maps/L_MainMenu"));
		DrainPersistence();
		Test.TestEqual(TEXT("Menu transition deduplicated"), AnalyticsSubsystem->State->SummarySubmitted, 1);
		Test.TestEqual(TEXT("Cross-map active duration"), SubmittedValues.FindRef(TEXT("Session:GameplayDuration")), 150.0);
		GameInstance->Shutdown();
		Test.TestEqual(TEXT("Shutdown callback deduplicated"), AnalyticsSubsystem->State->SummarySubmitted, 1);
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return true;
	}

	bool TestBoundsAndBlueprint(FAutomationTestBase& Test)
	{
		for (int32 Index = 0; Index < 1100; ++Index)
		{
			AnalyticsSubsystem->RecordEvent(FString::Printf(TEXT("Test:Key%d"), Index));
		}
		Test.TestEqual(TEXT("Identifier storage bounded"), AnalyticsSubsystem->State->Metrics.Num(), 1024);
		const TCHAR* const FunctionNames[] =
		{
			TEXT("RegisterMetric"),
			TEXT("RecordEvent"),
			TEXT("RecordValue"),
			TEXT("FlushAnalytics"),
			TEXT("EndGameplaySession")
		};
		for (const TCHAR* FunctionName : FunctionNames)
		{
			const UFunction* Function = AnalyticsSubsystem->FindFunction(FName(FunctionName));
			Test.TestTrue(FString::Printf(TEXT("BlueprintCallable %s"), FunctionName),
				Function && Function->HasAnyFunctionFlags(FUNC_BlueprintCallable));
		}
		AnalyticsSubsystem->State->Metrics.FindChecked(TEXT("Test:Key0")).Count = -10;
		AnalyticsSubsystem->ValidateState();
		Test.TestFalse(TEXT("Invalid persisted aggregate removed"), AnalyticsSubsystem->State->Metrics.Contains(TEXT("Test:Key0")));
		AnalyticsSubsystem->State->PeriodicSubmitted = -1;
		AnalyticsSubsystem->ValidateState();
		Test.TestEqual(TEXT("Invalid persisted budget fails closed"), AnalyticsSubsystem->State->PeriodicSubmitted, 470);
		AnalyticsSubsystem->State->Version = 1;
		AnalyticsSubsystem->State->Metrics.Empty();
		AnalyticsSubsystem->RegisterMetric(TEXT("Performance:MapLoad:Legacy"), EGameplayAnalyticsAggregation::Sum);
		AnalyticsSubsystem->RecordValue(TEXT("Performance:MapLoad:Legacy"), 10.f);
		AnalyticsSubsystem->ValidateState();
		Test.TestTrue(TEXT("Legacy scene samples migrate to mean"),
			AnalyticsSubsystem->State->Metrics.FindChecked(TEXT("Performance:MapLoad:Legacy")).Aggregation
				== EGameplayAnalyticsAggregation::Average);
		Test.TestEqual(TEXT("Migration retains samples"),
			AnalyticsSubsystem->State->Metrics.FindChecked(TEXT("Performance:MapLoad:Legacy")).Sum, 10.0);
		return true;
	}

	bool TestAsyncOrderingAndSnapshots(FAutomationTestBase& Test)
	{
		AnalyticsSubsystem->RecordValue(TEXT("Async:First"), 1.f);
		AnalyticsSubsystem->RecordValue(TEXT("Async:Second"), 2.f);
		AnalyticsSubsystem->AdvanceGameplayTime(300);
		TArray<TSharedPtr<TPromise<bool>>> SavePromises;
		TArray<TArray<uint8>> SavedSnapshots;
		AnalyticsSubsystem->TestAsyncSave = [&SavePromises, &SavedSnapshots](TArray<uint8>&& Bytes)
		{
			SavedSnapshots.Add(MoveTemp(Bytes));
			TSharedPtr<TPromise<bool>> Promise = MakeShared<TPromise<bool>>();
			SavePromises.Add(Promise);
			return Promise->GetFuture();
		};
		PumpPersistenceFrame(AnalyticsSubsystem);
		Test.TestEqual(TEXT("Only one async save starts"), SavePromises.Num(), 1);
		Test.TestEqual(TEXT("No SDK call before successful disk completion"), SubmissionCount, 0);
		UGameplayAnalyticsSaveGame* const SavedState = Cast<UGameplayAnalyticsSaveGame>(
			UGameplayStatics::LoadGameFromMemory(SavedSnapshots[0]));
		if (!Test.TestNotNull(TEXT("Immutable reservation snapshot deserializes"), SavedState))
		{
			SavePromises[0]->SetValue(false);
			return false;
		}
		Test.TestEqual(TEXT("Snapshot reserves one quota unit"), SavedState->PeriodicSubmitted, 1);
		Test.TestEqual(TEXT("Snapshot removes only the reserved observation"), SavedState->Metrics.FindChecked(TEXT("Async:First")).Count, int64(0));
		AnalyticsSubsystem->RecordValue(TEXT("Async:First"), 5.f);
		for (int32 Request = 0; Request < 100; ++Request)
		{
			AnalyticsSubsystem->FlushAnalytics();
			AnalyticsSubsystem->TickPersistence(0.0f);
		}
		Test.TestEqual(TEXT("Repeated requests do not start overlapping saves"), SavePromises.Num(), 1);
		Test.TestEqual(TEXT("Repeated requests do not increase reserved quota"), AnalyticsSubsystem->State->PeriodicSubmitted, 1);
		SavePromises[0]->SetValue(true);
		AnalyticsSubsystem->TickPersistence(0.0f);
		Test.TestEqual(TEXT("Same-frame completion cannot send"), SubmissionCount, 0);
		PumpPersistenceFrame(AnalyticsSubsystem);
		Test.TestEqual(TEXT("Successful reservation permits one SDK call"), SubmissionCount, 1);
		Test.TestEqual(TEXT("Completing a save does not start another in that frame"), SavePromises.Num(), 1);
		PumpPersistenceFrame(AnalyticsSubsystem);
		Test.TestEqual(TEXT("Next reservation starts on a later frame"), SavePromises.Num(), 2);
		Test.TestEqual(TEXT("Second event also waits for its disk write"), SubmissionCount, 1);
		SavePromises[1]->SetValue(true);
		PumpPersistenceFrame(AnalyticsSubsystem);
		Test.TestEqual(TEXT("Second completion permits its own event"), SubmissionCount, 2);
		Test.TestEqual(TEXT("Each observation uses exactly one quota unit"), AnalyticsSubsystem->State->PeriodicSubmitted, 2);
		Test.TestEqual(TEXT("First snapshot keeps its original value"), SubmittedValues.FindRef(TEXT("Async:First")), 1.0);
		Test.TestEqual(TEXT("New observations remain pending"), AnalyticsSubsystem->State->Metrics.FindChecked(TEXT("Async:First")).Sum, 5.0);
		AnalyticsSubsystem->TestAsyncSave = nullptr;
		DrainPersistence();
		Test.TestEqual(TEXT("An ID is only selected once in one flush cycle"), SubmissionCount, 2);
		return true;
	}

	bool TestAsyncDayRollover(FAutomationTestBase& Test)
	{
		AnalyticsSubsystem->RecordValue(TEXT("Async:Carry"), 42.f);
		AnalyticsSubsystem->AdvanceGameplayTime(300);
		TPromise<bool> SavePromise;
		AnalyticsSubsystem->TestAsyncSave = [&SavePromise](TArray<uint8>&&)
		{
			return SavePromise.GetFuture();
		};
		PumpPersistenceFrame(AnalyticsSubsystem);
		Test.TestEqual(TEXT("Old day reserves quota before I/O"), AnalyticsSubsystem->State->PeriodicSubmitted, 1);
		AnalyticsSubsystem->TestUtcDay = TEXT("2026-10-18");
		AnalyticsSubsystem->RecordValue(TEXT("Async:Carry"), 7.f);
		SavePromise.SetValue(true);
		PumpPersistenceFrame(AnalyticsSubsystem);
		Test.TestEqual(TEXT("Old reservation cannot send using yesterday's quota"), SubmissionCount, 0);
		Test.TestEqual(TEXT("Refund cannot decrement new-day quota"), AnalyticsSubsystem->State->PeriodicSubmitted, 0);
		Test.TestEqual(TEXT("Rollover merges older and newer observations"), AnalyticsSubsystem->State->Metrics.FindChecked(TEXT("Async:Carry")).Sum, 49.0);
		AnalyticsSubsystem->TestAsyncSave = nullptr;
		DrainPersistence();
		AnalyticsSubsystem->AdvanceGameplayTime(300);
		DrainPersistence();
		Test.TestEqual(TEXT("New day must earn its own allowance"), AnalyticsSubsystem->State->PeriodicSubmitted, 1);
		Test.TestEqual(TEXT("Carried data is reported once"), SubmittedValues.FindRef(TEXT("Async:Carry")), 49.0);
		return true;
	}

	bool TestAsyncEightHourCutoff(FAutomationTestBase& Test)
	{
		AnalyticsSubsystem->State->ActiveGameplaySeconds = 28800.0;
		AnalyticsSubsystem->State->PeriodicSubmitted = 469;
		AnalyticsSubsystem->RecordEvent(TEXT("Async:Cutoff"));
		AnalyticsSubsystem->FlushAnalytics();
		TPromise<bool> SavePromise;
		AnalyticsSubsystem->TestAsyncSave = [&SavePromise](TArray<uint8>&&)
		{
			return SavePromise.GetFuture();
		};
		PumpPersistenceFrame(AnalyticsSubsystem);
		Test.TestEqual(TEXT("Final reservation still obeys 470 periodic cap"), AnalyticsSubsystem->State->PeriodicSubmitted, 470);
		AnalyticsSubsystem->AdvanceGameplayTime(1);
		SavePromise.SetValue(true);
		PumpPersistenceFrame(AnalyticsSubsystem);
		Test.TestEqual(TEXT("Delayed callback cannot submit after eight hours"), SubmissionCount, 0);
		Test.TestEqual(TEXT("Cancelled reservation refunds only its own charge"), AnalyticsSubsystem->State->PeriodicSubmitted, 469);
		Test.TestEqual(TEXT("Cancelled event remains pending"), AnalyticsSubsystem->State->Metrics.FindChecked(TEXT("Async:Cutoff")).Count, int64(1));
		AnalyticsSubsystem->TestAsyncSave = nullptr;
		for (int32 Frame = 0; Frame < 100; ++Frame)
		{
			AnalyticsSubsystem->FlushAnalytics();
			PumpPersistenceFrame(AnalyticsSubsystem);
		}
		DrainPersistence();
		Test.TestEqual(TEXT("Extra frames and flushes never reopen the daily allowance"), AnalyticsSubsystem->State->PeriodicSubmitted, 469);
		Test.TestEqual(TEXT("Spacing cannot bypass eight-hour stop"), SubmissionCount, 0);
		return true;
	}

	bool TestAsyncFailureAndRetry(FAutomationTestBase& Test)
	{
		Test.AddExpectedMessagePlain(TEXT("Analytics checkpoint failed; retry deferred for one minute"), ELogVerbosity::Warning);
		AnalyticsSubsystem->RecordValue(TEXT("Async:Failure"), 1.f);
		AnalyticsSubsystem->AdvanceGameplayTime(300);
		TPromise<bool> SavePromise;
		int32 AsyncSaveCount = 0;
		AnalyticsSubsystem->TestAsyncSave = [&SavePromise, &AsyncSaveCount](TArray<uint8>&&)
		{
			++AsyncSaveCount;
			return SavePromise.GetFuture();
		};
		PumpPersistenceFrame(AnalyticsSubsystem);
		AnalyticsSubsystem->RecordValue(TEXT("Async:Failure"), 2.f);
		SavePromise.SetValue(false);
		PumpPersistenceFrame(AnalyticsSubsystem);
		Test.TestEqual(TEXT("Failed write cannot call SDK"), SubmissionCount, 0);
		Test.TestEqual(TEXT("Failed write restores quota"), AnalyticsSubsystem->State->PeriodicSubmitted, 0);
		Test.TestEqual(TEXT("Failed write preserves newer observations"), AnalyticsSubsystem->State->Metrics.FindChecked(TEXT("Async:Failure")).Sum, 3.0);
		for (int32 Frame = 0; Frame < 10; ++Frame)
		{
			AnalyticsSubsystem->FlushAnalytics();
			PumpPersistenceFrame(AnalyticsSubsystem, 1.0f);
		}
		Test.TestEqual(TEXT("Failure cooldown prevents a per-frame disk retry loop"), AsyncSaveCount, 1);
		AnalyticsSubsystem->TestAsyncSave = nullptr;
		PumpPersistenceFrame(AnalyticsSubsystem, 60.0f);
		DrainPersistence();
		Test.TestEqual(TEXT("Recovered provider submits one combined observation"), SubmittedValues.FindRef(TEXT("Async:Failure")), 3.0);
		Test.TestEqual(TEXT("Retry is charged exactly once"), AnalyticsSubsystem->State->PeriodicSubmitted, 1);
		return true;
	}

	bool TestAsyncShutdownOrdering(FAutomationTestBase& Test)
	{
		AnalyticsSubsystem->SlotName = TEXT("AnalyticsShutdown_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
		AnalyticsSubsystem->TestPersist = nullptr;
		AnalyticsSubsystem->RecordEvent(TEXT("Async:Shutdown"));
		AnalyticsSubsystem->AdvanceGameplayTime(300);
		PumpPersistenceFrame(AnalyticsSubsystem);
		Test.TestTrue(TEXT("A real worker write is outstanding before shutdown"), AnalyticsSubsystem->SaveFuture.IsValid());
		AnalyticsSubsystem->RecordEvent(TEXT("Async:Shutdown"));
		AnalyticsSubsystem->bShuttingDown = true;
		AnalyticsSubsystem->FinishPersistence();
		Test.TestFalse(TEXT("Shutdown joins the worker before its final save"), AnalyticsSubsystem->SaveFuture.IsValid());
		Test.TestEqual(TEXT("Shutdown sends only the reserved duration summary"), SubmissionCount, 1);
		Test.TestEqual(TEXT("Unsent periodic reservation is refunded"), AnalyticsSubsystem->State->PeriodicSubmitted, 0);
		Test.TestEqual(TEXT("Shutdown summary uses its own reserve"), AnalyticsSubsystem->State->SummarySubmitted, 1);
		UGameplayAnalyticsSaveGame* const SavedState = Cast<UGameplayAnalyticsSaveGame>(
			UGameplayStatics::LoadGameFromSlot(AnalyticsSubsystem->SlotName, 0));
		if (!Test.TestNotNull(TEXT("Final shutdown save is readable"), SavedState))
		{
			return false;
		}
		Test.TestEqual(TEXT("An older async snapshot cannot overwrite the final summary counter"), SavedState->SummarySubmitted, 1);
		Test.TestEqual(TEXT("Unsent and newer periodic data survive shutdown"), SavedState->Metrics.FindChecked(TEXT("Async:Shutdown")).Count, int64(2));
		Test.TestEqual(TEXT("Completed duration is not recovered again after restart"), SavedState->OpenSessionSeconds, 0.0);
		UGameplayStatics::DeleteGameInSlot(AnalyticsSubsystem->SlotName, 0);
		return true;
	}

	bool TestAsyncSpacingBudget(FAutomationTestBase& Test)
	{
		for (int32 Index = 0; Index < 32; ++Index)
		{
			AnalyticsSubsystem->RecordEvent(FString::Printf(TEXT("Async:Spacing:Metric%d"), Index));
		}
		AnalyticsSubsystem->AdvanceGameplayTime(299);
		AnalyticsSubsystem->FlushAnalytics();
		DrainPersistence();
		Test.TestEqual(TEXT("Async frames do not earn allowance before five active minutes"), SubmissionCount, 0);
		AnalyticsSubsystem->AdvanceGameplayTime(1);
		for (int32 Frame = 0; Frame < 100; ++Frame)
		{
			for (int32 Index = 0; Index < 32; ++Index)
			{
				AnalyticsSubsystem->RecordEvent(FString::Printf(TEXT("Async:Spacing:Metric%d"), Index));
			}
			AnalyticsSubsystem->FlushAnalytics();
			PumpPersistenceFrame(AnalyticsSubsystem);
		}
		DrainPersistence();
		for (int32 Request = 0; Request < 100; ++Request)
		{
			AnalyticsSubsystem->FlushAnalytics();
			DrainPersistence();
		}
		Test.TestEqual(TEXT("Save processing does not advance gameplay time"), AnalyticsSubsystem->State->ActiveGameplaySeconds, 300.0);
		Test.TestEqual(TEXT("Five active minutes still earn only six periodic events"), AnalyticsSubsystem->EarnedPeriodicAllowance(), 6);
		Test.TestEqual(TEXT("Extra frames and flush cycles cannot increase submissions"), SubmissionCount, 6);
		Test.TestEqual(TEXT("Persisted periodic usage matches actual submissions"), AnalyticsSubsystem->State->PeriodicSubmitted, 6);
		Test.TestEqual(TEXT("Periodic work cannot spend the summary reserve"), AnalyticsSubsystem->State->SummarySubmitted, 0);
		return true;
	}

	TStrongObjectPtr<UGameInstance> GameInstance;
	UGameplayAnalyticsSubsystem* AnalyticsSubsystem;
	TMap<FString, double> SubmittedValues;
	int32 SubmissionCount = 0;
};

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FGameplayAnalyticsManagerTest,
	"Atlantis.Analytics.Manager",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
void FGameplayAnalyticsManagerTest::GetTests(TArray<FString>& OutNames, TArray<FString>& OutCommands) const
{
	const TCHAR* const TestNames[] =
	{
		TEXT("Aggregation"),
		TEXT("BudgetAndFairness"),
		TEXT("FailureAndReentrancy"),
		TEXT("RolloverAndSessions"),
		TEXT("Persistence"),
		TEXT("BoundsAndBlueprint"),
		TEXT("WorldLifecycle"),
		TEXT("AsyncOrderingAndSnapshots"),
		TEXT("AsyncDayRollover"),
		TEXT("AsyncEightHourCutoff"),
		TEXT("AsyncFailureAndRetry"),
		TEXT("AsyncShutdownOrdering"),
		TEXT("AsyncSpacingBudget")
	};
	for (const TCHAR* CaseDisplayName : TestNames)
	{
		OutNames.Add(CaseDisplayName);
		OutCommands.Add(CaseDisplayName);
	}
}

bool FGameplayAnalyticsManagerTest::RunTest(const FString& Parameters)
{
	return FGameplayAnalyticsTestAccess::Run(*this, Parameters);
}
#endif
