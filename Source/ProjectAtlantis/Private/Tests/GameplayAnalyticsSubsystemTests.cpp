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
		CreatedManager->RefreshUtcDay();
		CreatedManager->TestPersist = []
		{
			return true;
		};
		return CreatedManager;
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
		Test.TestEqual(TEXT("Early manual flush earns nothing"), SubmissionCount, 0);
		AnalyticsSubsystem->AdvanceGameplayTime(299);
		Test.TestEqual(TEXT("No early scheduled send"), SubmissionCount, 0);
		AnalyticsSubsystem->AdvanceGameplayTime(1);
		Test.TestEqual(TEXT("Count"), SubmittedValues.FindRef(TEXT("Test:Count")), 3.0);
		Test.TestEqual(TEXT("Sum"), SubmittedValues.FindRef(TEXT("Test:Sum")), 18.0);
		Test.TestEqual(TEXT("Average is sample-weighted across pending intervals"), SubmittedValues.FindRef(TEXT("Test:Average")), 6.0);
		Test.TestEqual(TEXT("Latest"), SubmittedValues.FindRef(TEXT("Test:Latest")), 12.0);
		Test.TestEqual(TEXT("Performance mean milliseconds"), SubmittedValues.FindRef(TEXT("Performance:MapLoad:Test")), 20.0);
		AnalyticsSubsystem->FlushAnalytics();
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
		AnalyticsSubsystem->RecordEvent(TEXT("Test:AfterEight"));
		const int32 PreviousSubmissionCount = SubmissionCount;
		AnalyticsSubsystem->FlushAnalytics();
		Test.TestEqual(TEXT("No periodic submission after eight hours"), SubmissionCount, PreviousSubmissionCount);
		Test.TestEqual(
			TEXT("Post-cap observation retained"), AnalyticsSubsystem->State->Metrics.FindChecked(TEXT("Test:AfterEight")).Count, int64(1));
		return true;
	}

	bool TestFailureAndReentrancy(FAutomationTestBase& Test)
	{
		AnalyticsSubsystem->bTestProviderUnavailable = true;
		AnalyticsSubsystem->RecordValue(TEXT("Test:Value"), 5.f);
		AnalyticsSubsystem->AdvanceGameplayTime(300);
		Test.TestEqual(TEXT("Unavailable provider retains pending"),
			AnalyticsSubsystem->State->Metrics.FindChecked(TEXT("Test:Value")).Count, int64(1));
		AnalyticsSubsystem->bTestProviderUnavailable = false;
		AnalyticsSubsystem->TestSink = [this](const FString&, double)
		{
			++SubmissionCount;
			return false;
		};
		AnalyticsSubsystem->FlushAnalytics();
		Test.TestEqual(TEXT("Failed enqueue keeps pending"), AnalyticsSubsystem->State->Metrics.FindChecked(TEXT("Test:Value")).Sum, 5.0);
		Test.TestEqual(TEXT("Failed enqueue restores quota"), AnalyticsSubsystem->State->PeriodicSubmitted, 0);
		AnalyticsSubsystem->TestPersist = []
		{
			return false;
		};
		const int32 PreviousSubmissionCount = SubmissionCount;
		AnalyticsSubsystem->FlushAnalytics();
		Test.TestEqual(TEXT("Disk failure blocks SDK call"), SubmissionCount, PreviousSubmissionCount);
		int32 FailedCheckpoints = 0;
		AnalyticsSubsystem->CheckpointSeconds = 0.0;
		AnalyticsSubsystem->TestPersist = [&FailedCheckpoints]
		{
			++FailedCheckpoints;
			return false;
		};
		AnalyticsSubsystem->AdvanceGameplayTime(60);
		AnalyticsSubsystem->AdvanceGameplayTime(1);
		Test.TestEqual(TEXT("Disk failure does not retry every frame"), FailedCheckpoints, 1);
		AnalyticsSubsystem->TestPersist = []
		{
			return true;
		};
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
		Test.TestEqual(
			TEXT("New observation survives snapshot"), AnalyticsSubsystem->State->Metrics.FindChecked(TEXT("Test:Value")).Sum, 7.0);
		Test.TestEqual(TEXT("One reservation committed"), AnalyticsSubsystem->State->PeriodicSubmitted, 1);
		return true;
	}

	bool TestRolloverAndSessions(FAutomationTestBase& Test)
	{
		AnalyticsSubsystem->AdvanceGameplayTime(100);
		AnalyticsSubsystem->EndGameplaySession();
		AnalyticsSubsystem->EndGameplaySession();
		Test.TestEqual(TEXT("Duplicate lifecycle callback"), AnalyticsSubsystem->State->SummarySubmitted, 1);
		Test.TestEqual(TEXT("Duration seconds"), SubmittedValues.FindRef(TEXT("Session:GameplayDuration")), 100.0);
		AnalyticsSubsystem->AdvanceGameplayTime(200);
		Test.TestEqual(TEXT("Sessions share active day time"), AnalyticsSubsystem->State->ActiveGameplaySeconds, 300.0);
		AnalyticsSubsystem->State->SummarySubmitted = 10;
		AnalyticsSubsystem->EndGameplaySession();
		Test.TestEqual(TEXT("Excess summary deferred"), AnalyticsSubsystem->State->PendingSessionSeconds, 200.0);
		AnalyticsSubsystem->RecordValue(TEXT("Test:Pending"), 42.f);
		AnalyticsSubsystem->TestUtcDay = TEXT("2026-10-18");
		AnalyticsSubsystem->FlushAnalytics();
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
		Test.TestTrue(TEXT("Real SaveGame checkpoint"), AnalyticsSubsystem->Persist());
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
		AnalyticsSubsystem->OnPreLoadMap(*GameInstance->GetWorldContext(), TEXT("/Game/Maps/L_SecondGameplay"));
		Test.TestTrue(TEXT("Gameplay transition preserves session"), AnalyticsSubsystem->bSessionOpen);
		AnalyticsSubsystem->AdvanceGameplayTime(50);
		AnalyticsSubsystem->OnPreLoadMap(*GameInstance->GetWorldContext(), TEXT("/Game/Maps/L_MainMenu"));
		AnalyticsSubsystem->OnPreLoadMap(*GameInstance->GetWorldContext(), TEXT("/Game/Maps/L_MainMenu"));
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
		TEXT("WorldLifecycle")
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
