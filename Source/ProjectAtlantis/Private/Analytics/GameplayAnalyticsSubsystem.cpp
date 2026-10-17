// Copyright (c) 2026 ASGC
#include "Analytics/GameplayAnalyticsSubsystem.h"

#include "Analytics/ScenePerformanceAnalyticsSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameAnalytics.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/DateTime.h"
#include "Misc/PackageName.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogGameplayAnalytics, Log, All);

void UGameplayAnalyticsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UScenePerformanceAnalyticsSubsystem>();
#if WITH_EDITOR
	// PIE instances must not overwrite each other's or the packaged game's budget.
	SlotName += FString::Printf(TEXT("_Editor_%d"), GetGameInstance()->GetWorld() ? GetGameInstance()->GetWorld()->GetPackage()->GetPIEInstanceID() : 0);
#endif
	const bool bExistingSave = UGameplayStatics::DoesSaveGameExist(SlotName, 0);
	State = bExistingSave ? Cast<UGameplayAnalyticsSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0)) : nullptr;
	if (!State || State->Version < 1 || State->Version > 2)
	{
		State = NewObject<UGameplayAnalyticsSaveGame>(this);
		if (bExistingSave)
		{
			// Fail closed for this day instead of granting another allowance on corruption.
			State->UtcDay = GetUtcDay();
			State->PeriodicSubmitted = FirstPhaseBudget + SecondPhaseBudget;
			State->SummarySubmitted = SummaryBudget;
			UE_LOG(LogGameplayAnalytics, Warning, TEXT("Unreadable analytics save; budget disabled until next UTC day"));
		}
	}
	ValidateState();
	// Recover checkpointed duration after an unclean exit as a deferred summary.
	State->PendingSessionSeconds += State->OpenSessionSeconds;
	State->OpenSessionSeconds = 0.0;
	RefreshUtcDay();
	TickHandle = FWorldDelegates::OnWorldPostActorTick.AddUObject(this, &UGameplayAnalyticsSubsystem::OnWorldPostActorTick);
	PreLoadHandle = FCoreUObjectDelegates::PreLoadMapWithContext.AddUObject(this, &UGameplayAnalyticsSubsystem::OnPreLoadMap);
	bDirty = true;
}

void UGameplayAnalyticsSubsystem::Deinitialize()
{
	EndGameplaySession();
	FWorldDelegates::OnWorldPostActorTick.Remove(TickHandle);
	FCoreUObjectDelegates::PreLoadMapWithContext.Remove(PreLoadHandle);
	Persist();
	Super::Deinitialize();
}

FString UGameplayAnalyticsSubsystem::GetUtcDay() const
{
#if WITH_DEV_AUTOMATION_TESTS
	if (!TestUtcDay.IsEmpty()) { return TestUtcDay; }
#endif
	return FDateTime::UtcNow().ToString(TEXT("%Y-%m-%d"));
}

void UGameplayAnalyticsSubsystem::ValidateState()
{
	const bool bLegacy = State->Version == 1;
	if (State->PeriodicSubmitted < 0 || State->PeriodicSubmitted > FirstPhaseBudget + SecondPhaseBudget ||
		State->SummarySubmitted < 0 || State->SummarySubmitted > SummaryBudget ||
		!FMath::IsFinite(State->ActiveGameplaySeconds) || State->ActiveGameplaySeconds < 0.0)
	{
		State->UtcDay = GetUtcDay();
		State->PeriodicSubmitted = FirstPhaseBudget + SecondPhaseBudget;
		State->SummarySubmitted = SummaryBudget;
	}
	State->Version = 2;
	State->PeriodicSubmitted = FMath::Clamp(State->PeriodicSubmitted, 0, FirstPhaseBudget + SecondPhaseBudget);
	State->SummarySubmitted = FMath::Clamp(State->SummarySubmitted, 0, SummaryBudget);
	auto SanitizeSeconds = [](double Value) { return FMath::IsFinite(Value) ? FMath::Clamp(Value, 0.0, 315360000.0) : 0.0; };
	State->ActiveGameplaySeconds = SanitizeSeconds(State->ActiveGameplaySeconds);
	State->PendingSessionSeconds = SanitizeSeconds(State->PendingSessionSeconds);
	State->OpenSessionSeconds = SanitizeSeconds(State->OpenSessionSeconds);
	State->IntervalSeconds = FMath::Fmod(SanitizeSeconds(State->IntervalSeconds), IntervalSeconds);
	int32 Kept = 0;
	for (auto It = State->Metrics.CreateIterator(); It; ++It)
	{
		FGameplayAnalyticsMetric& Metric = It.Value();
		if (bLegacy && It.Key().StartsWith(TEXT("Performance:")) && Metric.Aggregation == EGameplayAnalyticsAggregation::Sum)
		{
			// Version 1 retained both sample count and sum, so migrate without losing samples.
			Metric.Aggregation = EGameplayAnalyticsAggregation::Average;
		}
		if (!IsEventIdValid(It.Key()) || It.Key() == TEXT("Session:GameplayDuration") ||
			static_cast<uint8>(Metric.Aggregation) > static_cast<uint8>(EGameplayAnalyticsAggregation::Latest) ||
			Metric.Count < 0 || !FMath::IsFinite(Metric.Sum) || !FMath::IsFinite(Metric.Latest) ||
			!FMath::IsFinite(Metric.LastSubmittedGameplaySeconds) || Kept >= MaxMetricKeys)
		{
			It.RemoveCurrent();
			continue;
		}
		Metric.Priority = FMath::Clamp(Metric.Priority, -100, 100);
		Metric.LastSubmittedGameplaySeconds = FMath::Clamp(Metric.LastSubmittedGameplaySeconds, 0.0, State->ActiveGameplaySeconds);
		++Kept;
	}
}

bool UGameplayAnalyticsSubsystem::IsEventIdValid(const FString& EventId) const
{
	if (EventId.IsEmpty() || EventId.Len() > 164) { return false; }
	TArray<FString> Parts;
	EventId.ParseIntoArray(Parts, TEXT(":"), false);
	if (Parts.Num() < 1 || Parts.Num() > 5) { return false; }
	for (const FString& Part : Parts)
	{
		if (Part.IsEmpty() || Part.Len() > 32) { return false; }
		for (TCHAR C : Part)
		{
			const bool bAsciiAlnum = (C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z') || (C >= '0' && C <= '9');
			if (!bAsciiAlnum && C != '_' && C != '-' && C != '.') { return false; }
		}
	}
	return true;
}

bool UGameplayAnalyticsSubsystem::RegisterMetric(const FString& EventId, EGameplayAnalyticsAggregation Aggregation, int32 Priority)
{
	if (!State || !IsEventIdValid(EventId) || EventId == TEXT("Session:GameplayDuration") ||
		static_cast<uint8>(Aggregation) > static_cast<uint8>(EGameplayAnalyticsAggregation::Latest)) { return false; }
	if (FGameplayAnalyticsMetric* Existing = State->Metrics.Find(EventId)) { return Existing->Aggregation == Aggregation; }
	if (State->Metrics.Num() >= MaxMetricKeys) { return false; }
	FGameplayAnalyticsMetric Metric;
	Metric.Aggregation = Aggregation;
	Metric.Priority = FMath::Clamp(Priority, -100, 100);
	// New identifiers do not get an artificial eight-hour age advantage.
	Metric.LastSubmittedGameplaySeconds = State->ActiveGameplaySeconds;
	State->Metrics.Add(EventId, Metric);
	bDirty = true;
	return true;
}

void UGameplayAnalyticsSubsystem::RecordEvent(const FString& EventId)
{
	if (!State) { return; }
	RefreshUtcDay();
	if (!State->Metrics.Contains(EventId) && !RegisterMetric(EventId, EGameplayAnalyticsAggregation::Count)) { return; }
	FGameplayAnalyticsMetric& Metric = State->Metrics.FindChecked(EventId);
	if (Metric.Aggregation != EGameplayAnalyticsAggregation::Count) { return; }
	if (Metric.Count < MAX_int64) { ++Metric.Count; }
	bDirty = true;
}

void UGameplayAnalyticsSubsystem::RecordValue(const FString& EventId, float Value)
{
	if (!State || !FMath::IsFinite(Value)) { return; }
	RefreshUtcDay();
	if (!State->Metrics.Contains(EventId) && !RegisterMetric(EventId, EGameplayAnalyticsAggregation::Sum)) { return; }
	FGameplayAnalyticsMetric& Metric = State->Metrics.FindChecked(EventId);
	if (Metric.Aggregation == EGameplayAnalyticsAggregation::Count) { return; }
	// Keep pending storage fixed-size even if the provider remains unavailable indefinitely.
	if (Metric.Count == MAX_int64) { return; }
	++Metric.Count;
	Metric.Sum += static_cast<double>(Value);
	Metric.Latest = Value;
	bDirty = true;
}

void UGameplayAnalyticsSubsystem::RecordPerformanceValue(const FString& EventId, float Value)
{
	// Load durations represent a mean in milliseconds, not the sum of repeated loads.
	if (RegisterMetric(EventId, EGameplayAnalyticsAggregation::Average)) { RecordValue(EventId, Value); }
}

bool UGameplayAnalyticsSubsystem::IsActiveGameplayWorld(const UWorld* World) const
{
	if (!World || !World->IsGameWorld() || World->GetGameInstance() != GetGameInstance() ||
		World != GetGameInstance()->GetWorld() || World->IsPaused() || World->GetNetMode() == NM_DedicatedServer) { return false; }
	const FString Map = UWorld::RemovePIEPrefix(FPackageName::GetShortName(World->GetMapName()));
	return !Map.Contains(TEXT("MainMenu")) && Map != TEXT("Entry");
}

void UGameplayAnalyticsSubsystem::OnWorldPostActorTick(UWorld* World, ELevelTick TickType, float DeltaSeconds)
{
	if (!IsActiveGameplayWorld(World) || TickType != LEVELTICK_All) { return; }
	// Engine frame time is undilated; world DeltaSeconds changes with gameplay time dilation.
	AdvanceGameplayTime(FApp::GetDeltaTime());
}

void UGameplayAnalyticsSubsystem::AdvanceGameplayTime(double Seconds)
{
	if (!State || !FMath::IsFinite(Seconds) || Seconds <= 0.0) { return; }
	RefreshUtcDay();
	bSessionOpen = true;
	// Split long/simulated ticks at reporting boundaries, including the final eight-hour boundary.
	while (Seconds > 0.0)
	{
		const double Step = FMath::Min(Seconds, IntervalSeconds - State->IntervalSeconds);
		State->ActiveGameplaySeconds += Step;
		State->OpenSessionSeconds += Step;
		State->IntervalSeconds += Step;
		CheckpointSeconds += Step;
		Seconds -= Step;
		bDirty = true;
		if (State->IntervalSeconds >= IntervalSeconds)
		{
			State->IntervalSeconds = 0.0;
			FlushAnalytics();
		}
	}
	// One checkpoint/minute, independent of producer frequency and SDK availability.
	if (bDirty && CheckpointSeconds >= 60.0)
	{
		CheckpointSeconds = 0.0;
		if (Persist()) { bDirty = false; }
	}
}

void UGameplayAnalyticsSubsystem::OnPreLoadMap(const FWorldContext& Context, const FString& MapName)
{
	if (Context.OwningGameInstance != GetGameInstance()) { return; }
	const FString Map = FPackageName::GetShortName(MapName);
	if (Map.Contains(TEXT("MainMenu")) || Map == TEXT("Entry")) { EndGameplaySession(); }
	else { Persist(); }
}

void UGameplayAnalyticsSubsystem::RefreshUtcDay()
{
	if (!State) { return; }
	const FString Today = GetUtcDay();
	// A backwards wall-clock adjustment must not replenish a consumed day.
	if (!State->UtcDay.IsEmpty() && State->UtcDay >= Today) { return; }
	State->UtcDay = Today;
	State->PeriodicSubmitted = 0;
	State->SummarySubmitted = 0;
	State->ActiveGameplaySeconds = 0.0;
	State->IntervalSeconds = 0.0;
	for (auto& Pair : State->Metrics) { Pair.Value.LastSubmittedGameplaySeconds = 0.0; }
	bDirty = true;
}

int32 UGameplayAnalyticsSubsystem::EarnedPeriodicAllowance() const
{
	if (!State) { return 0; }
	const double Active = State->ActiveGameplaySeconds;
	const int32 FirstIntervals = FMath::Clamp(FMath::FloorToInt(FMath::Min(Active, FirstPhaseSeconds) / IntervalSeconds), 0, 48);
	const int32 LaterIntervals = FMath::Clamp(FMath::FloorToInt(FMath::Max(0.0, FMath::Min(Active, EndPhaseSeconds) - FirstPhaseSeconds) / IntervalSeconds), 0, 48);
	return (FirstIntervals * FirstPhaseBudget) / 48 + (LaterIntervals * SecondPhaseBudget) / 48;
}

bool UGameplayAnalyticsSubsystem::CanSubmit() const
{
#if WITH_DEV_AUTOMATION_TESTS
	if (bTestProviderUnavailable) { return false; }
	if (TestSink) { return true; }
#endif
#if !WITH_EDITOR
	const UScenePerformanceAnalyticsSubsystem* Scene = GetGameInstance()->GetSubsystem<UScenePerformanceAnalyticsSubsystem>();
	return Scene && Scene->IsAnalyticsReady() && UGameAnalytics::GetInstance();
#else
	return false;
#endif
}

bool UGameplayAnalyticsSubsystem::SubmitValue(const FString& Id, double Value) const
{
#if WITH_DEV_AUTOMATION_TESTS
	if (TestSink) { return TestSink(Id, Value); }
#endif
#if !WITH_EDITOR
	if (CanSubmit())
	{
		UGameAnalytics::GetInstance()->AddDesignEventWithValue(Id, static_cast<float>(Value));
		return true; // The SDK has a void enqueue API, not a delivery receipt.
	}
#endif
	return false;
}

void UGameplayAnalyticsSubsystem::FlushPendingWithBudget(int32 Available)
{
	struct FCandidate { FString Id; double Score; };
	TArray<FCandidate> Candidates;
	for (const auto& Pair : State->Metrics)
	{
		if (Pair.Value.Count <= 0) { continue; }
		const double Wait = FMath::Max(0.0, State->ActiveGameplaySeconds - Pair.Value.LastSubmittedGameplaySeconds);
		Candidates.Add({Pair.Key, Wait + Pair.Value.Priority * 30.0});
	}
	Candidates.Sort([](const FCandidate& A, const FCandidate& B) { return A.Score == B.Score ? A.Id < B.Id : A.Score > B.Score; });
	for (const FCandidate& Candidate : Candidates)
	{
		if (Available <= 0 || !CanSubmit()) { break; }
		const FGameplayAnalyticsMetric Snapshot = State->Metrics.FindChecked(Candidate.Id);
		double Value = 0.0;
		switch (Snapshot.Aggregation)
		{
		case EGameplayAnalyticsAggregation::Count: Value = static_cast<double>(Snapshot.Count); break;
		case EGameplayAnalyticsAggregation::Sum: Value = Snapshot.Sum; break;
		case EGameplayAnalyticsAggregation::Average: Value = Snapshot.Sum / Snapshot.Count; break;
		case EGameplayAnalyticsAggregation::Latest: Value = Snapshot.Latest; break;
		}
		if (!FMath::IsFinite(Value) || FMath::Abs(Value) > MAX_flt) { continue; }
		// Checkpoint the reservation and removal BEFORE SDK enqueue. A crash cannot replay
		// an event already enqueued. A crash between checkpoint and enqueue can lose it.
		FGameplayAnalyticsMetric& Pending = State->Metrics.FindChecked(Candidate.Id);
		Pending.Count = 0;
		Pending.Sum = 0.0;
		Pending.Latest = 0.0;
		Pending.LastSubmittedGameplaySeconds = State->ActiveGameplaySeconds;
		++State->PeriodicSubmitted;
		if (!Persist() || !SubmitValue(Candidate.Id, Value))
		{
			// Reacquire after callback: a producer can rehash the map during submission.
			FGameplayAnalyticsMetric& Restore = State->Metrics.FindChecked(Candidate.Id);
			if (Restore.Count == 0) { Restore.Latest = Snapshot.Latest; }
			Restore.Count += Snapshot.Count;
			Restore.Sum += Snapshot.Sum;
			Restore.LastSubmittedGameplaySeconds = Snapshot.LastSubmittedGameplaySeconds;
			--State->PeriodicSubmitted;
			bDirty = true;
			Persist();
			break;
		}
		--Available;
	}
}

void UGameplayAnalyticsSubsystem::FlushAnalytics()
{
	if (!State || bFlushing) { return; }
	TGuardValue<bool> Guard(bFlushing, true);
	RefreshUtcDay();
	if (CanSubmit())
	{
		if (State->ActiveGameplaySeconds <= EndPhaseSeconds)
		{
			FlushPendingWithBudget(FMath::Max(0, EarnedPeriodicAllowance() - State->PeriodicSubmitted));
		}
		SubmitSessionSummary();
	}
	// Explicit flush also checkpoints observations when no allowance/provider is available.
	if (bDirty && Persist()) { bDirty = false; }
}

void UGameplayAnalyticsSubsystem::SubmitSessionSummary()
{
	if (!CanSubmit() || State->PendingSessionSeconds <= 0.0 || State->SummarySubmitted >= SummaryBudget) { return; }
	const double Duration = State->PendingSessionSeconds;
	State->PendingSessionSeconds = 0.0;
	++State->SummarySubmitted;
	if (!Persist() || !SubmitValue(TEXT("Session:GameplayDuration"), Duration))
	{
		State->PendingSessionSeconds += Duration;
		--State->SummarySubmitted;
		bDirty = true;
		Persist();
	}
}

void UGameplayAnalyticsSubsystem::EndGameplaySession()
{
	if (!State) { return; }
	RefreshUtcDay();
	if (bSessionOpen)
	{
		bSessionOpen = false;
		State->PendingSessionSeconds += State->OpenSessionSeconds;
		State->OpenSessionSeconds = 0.0;
		bDirty = true;
	}
	FlushAnalytics();
}

bool UGameplayAnalyticsSubsystem::Persist() const
{
#if WITH_DEV_AUTOMATION_TESTS
	if (TestPersist) { return TestPersist(); }
#endif
	if (!State) { return false; }
	const bool bSaved = UGameplayStatics::SaveGameToSlot(State, SlotName, 0);
	if (!bSaved) { UE_LOG(LogGameplayAnalytics, Warning, TEXT("Analytics checkpoint failed; submissions deferred")); }
	return bSaved;
}
