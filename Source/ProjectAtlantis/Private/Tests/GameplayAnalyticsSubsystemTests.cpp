// Copyright (c) 2026 ASGC
#if WITH_DEV_AUTOMATION_TESTS
#include "Analytics/GameplayAnalyticsSubsystem.h"
#include "Analytics/ScenePerformanceAnalyticsSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/PlayerState.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/Guid.h"
#include "UObject/StrongObjectPtr.h"

struct FGameplayAnalyticsTestAccess
{
	static UGameplayAnalyticsSubsystem* Create(UGameInstance* GameInstance)
	{
		auto* Manager = NewObject<UGameplayAnalyticsSubsystem>(GameInstance);
		Manager->State = NewObject<UGameplayAnalyticsSaveGame>(Manager);
		Manager->TestUtcDay = TEXT("2026-10-17");
		Manager->RefreshUtcDay();
		Manager->TestPersist = [] { return true; };
		return Manager;
	}

	static bool Run(FAutomationTestBase& Test, const FString& Case)
	{
		TStrongObjectPtr<UGameInstance> Instance(NewObject<UGameInstance>(GEngine));
		auto* M = Create(Instance.Get());
		TMap<FString, double> Sent;
		int32 Calls = 0;
		M->TestSink = [&](const FString& Id, double Value) { Sent.Add(Id, Value); ++Calls; return true; };
		if (Case == TEXT("Aggregation"))
		{
			M->RegisterMetric(TEXT("Test:Average"), EGameplayAnalyticsAggregation::Average);
			M->RegisterMetric(TEXT("Test:Latest"), EGameplayAnalyticsAggregation::Latest);
			Test.TestFalse(TEXT("Cannot reinterpret existing key"), M->RegisterMetric(TEXT("Test:Average"), EGameplayAnalyticsAggregation::Sum));
			Test.TestFalse(TEXT("Reserved summary ID"), M->RegisterMetric(TEXT("Session:GameplayDuration"), EGameplayAnalyticsAggregation::Count));
			Test.TestFalse(TEXT("Empty hierarchy segment"), M->RegisterMetric(TEXT("Test::Empty"), EGameplayAnalyticsAggregation::Count));
			for (float Value : {2.f, 4.f, 12.f})
			{
				M->RecordEvent(TEXT("Test:Count"));
				M->RecordValue(TEXT("Test:Sum"), Value);
				M->RecordValue(TEXT("Test:Average"), Value);
				M->RecordValue(TEXT("Test:Latest"), Value);
			}
			M->RecordPerformanceValue(TEXT("Performance:MapLoad:Test"), 10.f);
			M->RecordPerformanceValue(TEXT("Performance:MapLoad:Test"), 30.f);
			Test.TestEqual(TEXT("Recording does not submit"), Calls, 0);
			M->FlushAnalytics();
			Test.TestEqual(TEXT("Early manual flush earns nothing"), Calls, 0);
			M->AdvanceGameplayTime(299);
			Test.TestEqual(TEXT("No early scheduled send"), Calls, 0);
			M->AdvanceGameplayTime(1);
			Test.TestEqual(TEXT("Count"), Sent.FindRef(TEXT("Test:Count")), 3.0);
			Test.TestEqual(TEXT("Sum"), Sent.FindRef(TEXT("Test:Sum")), 18.0);
			Test.TestEqual(TEXT("Average is sample-weighted across pending intervals"), Sent.FindRef(TEXT("Test:Average")), 6.0);
			Test.TestEqual(TEXT("Latest"), Sent.FindRef(TEXT("Test:Latest")), 12.0);
			Test.TestEqual(TEXT("Performance mean milliseconds"), Sent.FindRef(TEXT("Performance:MapLoad:Test")), 20.0);
			M->FlushAnalytics();
			Test.TestEqual(TEXT("Unchanged metrics not resent"), Calls, 5);
		}
		else if (Case == TEXT("BudgetAndFairness"))
		{
			for (int32 Index = 0; Index < 100; ++Index)
			{
				M->RegisterMetric(FString::Printf(TEXT("Test:M%d"), Index), EGameplayAnalyticsAggregation::Count, Index == 99 ? -100 : 100);
			}
			for (int32 Interval = 0; Interval < 96; ++Interval)
			{
				for (int32 Index = 0; Index < 100; ++Index) { M->RecordEvent(FString::Printf(TEXT("Test:M%d"), Index)); }
				M->AdvanceGameplayTime(300);
				M->FlushAnalytics();
				M->EndGameplaySession(); // repeated short sessions cannot replenish periodic allowance
				if (Interval == 47) { Test.TestEqual(TEXT("Four-hour phase cap"), M->State->PeriodicSubmitted, 320); }
			}
			Test.TestEqual(TEXT("Eight-hour phase cap"), M->State->PeriodicSubmitted, 470);
			Test.TestEqual(TEXT("Separate summary reserve"), M->State->SummarySubmitted, 10);
			Test.TestTrue(TEXT("Aging eventually serves lowest priority"), Sent.Contains(TEXT("Test:M99")));
			Test.TestTrue(TEXT("Combined day cap"), Calls <= 480);
			M->AdvanceGameplayTime(300);
			M->RecordEvent(TEXT("Test:AfterEight"));
			const int32 Before = Calls;
			M->FlushAnalytics();
			Test.TestEqual(TEXT("No periodic submission after eight hours"), Calls, Before);
			Test.TestEqual(TEXT("Post-cap observation retained"), M->State->Metrics.FindChecked(TEXT("Test:AfterEight")).Count, int64(1));
		}
		else if (Case == TEXT("FailureAndReentrancy"))
		{
			M->bTestProviderUnavailable = true;
			M->RecordValue(TEXT("Test:Value"), 5.f);
			M->AdvanceGameplayTime(300);
			Test.TestEqual(TEXT("Unavailable provider retains pending"), M->State->Metrics.FindChecked(TEXT("Test:Value")).Count, int64(1));
			M->bTestProviderUnavailable = false;
			M->TestSink = [&](const FString&, double) { ++Calls; return false; };
			M->FlushAnalytics();
			Test.TestEqual(TEXT("Failed enqueue keeps pending"), M->State->Metrics.FindChecked(TEXT("Test:Value")).Sum, 5.0);
			Test.TestEqual(TEXT("Failed enqueue restores quota"), M->State->PeriodicSubmitted, 0);
			M->TestPersist = [] { return false; };
			const int32 Before = Calls;
			M->FlushAnalytics();
			Test.TestEqual(TEXT("Disk failure blocks SDK call"), Calls, Before);
			int32 FailedCheckpoints = 0;
			M->CheckpointSeconds = 0.0;
			M->TestPersist = [&] { ++FailedCheckpoints; return false; };
			M->AdvanceGameplayTime(60);
			M->AdvanceGameplayTime(1);
			Test.TestEqual(TEXT("Disk failure does not retry every frame"), FailedCheckpoints, 1);
			M->TestPersist = [] { return true; };
			M->TestSink = [&](const FString& Id, double Value)
			{
				Test.TestEqual(TEXT("Snapshot numeric value"), Value, 5.0);
				M->RecordValue(Id, 7.f);
				for (int32 Index = 0; Index < 100; ++Index) { M->RegisterMetric(FString::Printf(TEXT("Rehash:M%d"), Index), EGameplayAnalyticsAggregation::Count); }
				M->FlushAnalytics(); // recursion must not resend the snapshot
				++Calls;
				return true;
			};
			M->FlushAnalytics();
			Test.TestEqual(TEXT("New observation survives snapshot"), M->State->Metrics.FindChecked(TEXT("Test:Value")).Sum, 7.0);
			Test.TestEqual(TEXT("One reservation committed"), M->State->PeriodicSubmitted, 1);
		}
		else if (Case == TEXT("RolloverAndSessions"))
		{
			M->AdvanceGameplayTime(100);
			M->EndGameplaySession();
			M->EndGameplaySession();
			Test.TestEqual(TEXT("Duplicate lifecycle callback"), M->State->SummarySubmitted, 1);
			Test.TestEqual(TEXT("Duration seconds"), Sent.FindRef(TEXT("Session:GameplayDuration")), 100.0);
			M->AdvanceGameplayTime(200);
			Test.TestEqual(TEXT("Sessions share active day time"), M->State->ActiveGameplaySeconds, 300.0);
			M->State->SummarySubmitted = 10;
			M->EndGameplaySession();
			Test.TestEqual(TEXT("Excess summary deferred"), M->State->PendingSessionSeconds, 200.0);
			M->RecordValue(TEXT("Test:Pending"), 42.f);
			M->TestUtcDay = TEXT("2026-10-18");
			M->FlushAnalytics();
			Test.TestEqual(TEXT("UTC quota resets"), M->State->PeriodicSubmitted, 0);
			Test.TestEqual(TEXT("UTC active time resets"), M->State->ActiveGameplaySeconds, 0.0);
			Test.TestEqual(TEXT("Deferred summary retried"), M->State->SummarySubmitted, 1);
			Test.TestEqual(TEXT("Pending metric crosses day"), M->State->Metrics.FindChecked(TEXT("Test:Pending")).Sum, 42.0);
			M->TestUtcDay = TEXT("2026-10-17");
			M->RefreshUtcDay();
			Test.TestEqual(TEXT("Backwards clock does not replenish"), M->State->SummarySubmitted, 1);
		}
		else if (Case == TEXT("Persistence"))
		{
			M->SlotName = TEXT("AnalyticsAutomation_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
			M->TestPersist = nullptr;
			M->RecordEvent(TEXT("Test:Pending"));
			M->AdvanceGameplayTime(150);
			Test.TestTrue(TEXT("Real SaveGame checkpoint"), M->Persist());
			auto* Restored = Create(Instance.Get());
			Restored->State = Cast<UGameplayAnalyticsSaveGame>(UGameplayStatics::LoadGameFromSlot(M->SlotName, 0));
			if (!Test.TestNotNull(TEXT("SaveGame restored"), Restored->State.Get())) { return false; }
			Restored->ValidateState();
			Test.TestEqual(TEXT("Pending data survives restart"), Restored->State->Metrics.FindChecked(TEXT("Test:Pending")).Count, int64(1));
			Test.TestEqual(TEXT("Open session survives checkpoint"), Restored->State->OpenSessionSeconds, 150.0);
			Test.TestEqual(TEXT("Scheduler survives restart"), Restored->State->IntervalSeconds, 150.0);
			M->AdvanceGameplayTime(150); // submits and persists reservation
			Restored->State = Cast<UGameplayAnalyticsSaveGame>(UGameplayStatics::LoadGameFromSlot(M->SlotName, 0));
			Test.TestEqual(TEXT("Sent snapshot not replayed after restart"), Restored->State->Metrics.FindChecked(TEXT("Test:Pending")).Count, int64(0));
			Test.TestEqual(TEXT("Quota survives restart"), Restored->State->PeriodicSubmitted, 1);
			UGameplayStatics::DeleteGameInSlot(M->SlotName, 0);
		}
		else if (Case == TEXT("WorldLifecycle"))
		{
			Instance->InitializeStandalone();
			M = Instance->GetSubsystem<UGameplayAnalyticsSubsystem>();
			if (!Test.TestNotNull(TEXT("Automatic GameInstance subsystem initialization"), M)) { return false; }
			M->State = NewObject<UGameplayAnalyticsSaveGame>(M);
			M->TestPersist = [] { return true; };
			M->TestUtcDay = TEXT("2026-10-17");
			M->TestSink = [&](const FString& Id, double Value) { Sent.Add(Id, Value); ++Calls; return true; };
			UWorld* World = Instance->GetWorld();
			Test.TestTrue(TEXT("Primary gameplay world accepted"), M->IsActiveGameplayWorld(World));
			World->GetWorldSettings()->SetPauserPlayerState(World->SpawnActor<APlayerState>());
			Test.TestFalse(TEXT("Paused world excluded"), M->IsActiveGameplayWorld(World));
			World->GetWorldSettings()->SetPauserPlayerState(nullptr);
			Test.TestTrue(TEXT("Resumed world accepted"), M->IsActiveGameplayWorld(World));
			M->AdvanceGameplayTime(100);
			M->OnPreLoadMap(*Instance->GetWorldContext(), TEXT("/Game/Maps/L_SecondGameplay"));
			Test.TestTrue(TEXT("Gameplay transition preserves session"), M->bSessionOpen);
			M->AdvanceGameplayTime(50);
			M->OnPreLoadMap(*Instance->GetWorldContext(), TEXT("/Game/Maps/L_MainMenu"));
			M->OnPreLoadMap(*Instance->GetWorldContext(), TEXT("/Game/Maps/L_MainMenu"));
			Test.TestEqual(TEXT("Menu transition deduplicated"), M->State->SummarySubmitted, 1);
			Test.TestEqual(TEXT("Cross-map active duration"), Sent.FindRef(TEXT("Session:GameplayDuration")), 150.0);
			Instance->Shutdown();
			Test.TestEqual(TEXT("Shutdown callback deduplicated"), M->State->SummarySubmitted, 1);
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}
		else if (Case == TEXT("BoundsAndBlueprint"))
		{
			for (int32 Index = 0; Index < 1100; ++Index) { M->RecordEvent(FString::Printf(TEXT("Test:Key%d"), Index)); }
			Test.TestEqual(TEXT("Identifier storage bounded"), M->State->Metrics.Num(), 1024);
			for (const TCHAR* Name : {TEXT("RegisterMetric"), TEXT("RecordEvent"), TEXT("RecordValue"), TEXT("FlushAnalytics"), TEXT("EndGameplaySession")})
			{
				UFunction* Function = M->FindFunction(FName(Name));
				Test.TestTrue(FString::Printf(TEXT("BlueprintCallable %s"), Name), Function && Function->HasAnyFunctionFlags(FUNC_BlueprintCallable));
			}
			M->State->Metrics.FindChecked(TEXT("Test:Key0")).Count = -10;
			M->ValidateState();
			Test.TestFalse(TEXT("Invalid persisted aggregate removed"), M->State->Metrics.Contains(TEXT("Test:Key0")));
			M->State->PeriodicSubmitted = -1;
			M->ValidateState();
			Test.TestEqual(TEXT("Invalid persisted budget fails closed"), M->State->PeriodicSubmitted, 470);
			M->State->Version = 1;
			M->State->Metrics.Empty();
			M->RegisterMetric(TEXT("Performance:MapLoad:Legacy"), EGameplayAnalyticsAggregation::Sum);
			M->RecordValue(TEXT("Performance:MapLoad:Legacy"), 10.f);
			M->ValidateState();
			Test.TestTrue(TEXT("Legacy scene samples migrate to mean"), M->State->Metrics.FindChecked(TEXT("Performance:MapLoad:Legacy")).Aggregation == EGameplayAnalyticsAggregation::Average);
			Test.TestEqual(TEXT("Migration retains samples"), M->State->Metrics.FindChecked(TEXT("Performance:MapLoad:Legacy")).Sum, 10.0);
		}
		return true;
	}
};

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FGameplayAnalyticsManagerTest, "Atlantis.Analytics.Manager", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
void FGameplayAnalyticsManagerTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	for (const TCHAR* Name : {TEXT("Aggregation"), TEXT("BudgetAndFairness"), TEXT("FailureAndReentrancy"), TEXT("RolloverAndSessions"), TEXT("Persistence"), TEXT("BoundsAndBlueprint"), TEXT("WorldLifecycle")})
	{
		Names.Add(Name);
		Commands.Add(Name);
	}
}
bool FGameplayAnalyticsManagerTest::RunTest(const FString& Parameters)
{
	return FGameplayAnalyticsTestAccess::Run(*this, Parameters);
}
#endif
