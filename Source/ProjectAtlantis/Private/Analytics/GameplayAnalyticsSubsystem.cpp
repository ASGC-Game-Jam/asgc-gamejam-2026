// Copyright (c) 2026 ASGC
#include "Analytics/GameplayAnalyticsSubsystem.h"

#include "Analytics/ScenePerformanceAnalyticsSubsystem.h"
#include "Async/Async.h"
#include "CoreGlobals.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameAnalytics.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/DateTime.h"
#include "Misc/PackageName.h"
#include "PlatformFeatures.h"
#include "SaveGameSystem.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogGameplayAnalytics, Log, All);

namespace GameplayAnalytics
{
	constexpr int32 OldestSaveVersion = 1;
	constexpr int32 MaxPriority = 100;
	constexpr int32 MaxEventSegments = 5;
	constexpr int32 MaxEventSegmentLength = 32;
	constexpr int32 MaxEventIdLength = MaxEventSegments * (MaxEventSegmentLength + 1) - 1;
	constexpr int32 ReportingIntervalsPerPhase = 48;
	constexpr double CheckpointIntervalSeconds = 60.0;
	constexpr double PriorityAgeBonusSeconds = 30.0;
	constexpr double MaxPersistedDurationSeconds = 10.0 * 365.0 * 24.0 * 60.0 * 60.0;
}

void UGameplayAnalyticsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UScenePerformanceAnalyticsSubsystem>();
#if WITH_EDITOR
	// PIE instances must not overwrite each other's or the packaged game's budget.
	SlotName += FString::Printf(
		TEXT("_Editor_%d"), GetGameInstance()->GetWorld() ? GetGameInstance()->GetWorld()->GetPackage()->GetPIEInstanceID() : 0);
#endif
	const bool bExistingSave = UGameplayStatics::DoesSaveGameExist(SlotName, 0);
	State = bExistingSave ? Cast<UGameplayAnalyticsSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0)) : nullptr;
	if (!State || State->Version < GameplayAnalytics::OldestSaveVersion || State->Version > UGameplayAnalyticsSaveGame::CurrentVersion)
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
	PersistenceTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UGameplayAnalyticsSubsystem::TickPersistence));
}

void UGameplayAnalyticsSubsystem::Deinitialize()
{
	bShuttingDown = true;
	FTSTicker::RemoveTicker(PersistenceTickerHandle);
	FWorldDelegates::OnWorldPostActorTick.Remove(TickHandle);
	FCoreUObjectDelegates::PreLoadMapWithContext.Remove(PreLoadHandle);
	FinishPersistence();
	Super::Deinitialize();
}

FString UGameplayAnalyticsSubsystem::GetUtcDay() const
{
#if WITH_DEV_AUTOMATION_TESTS
	if (!TestUtcDay.IsEmpty())
	{
		return TestUtcDay;
	}
#endif
	return FDateTime::UtcNow().ToString(TEXT("%Y-%m-%d"));
}

void UGameplayAnalyticsSubsystem::ValidateState()
{
	const bool bLegacy = State->Version == GameplayAnalytics::OldestSaveVersion;
	if (State->PeriodicSubmitted < 0 || State->PeriodicSubmitted > FirstPhaseBudget + SecondPhaseBudget || State->SummarySubmitted < 0
		|| State->SummarySubmitted > SummaryBudget || !FMath::IsFinite(State->ActiveGameplaySeconds) || State->ActiveGameplaySeconds < 0.0)
	{
		State->UtcDay = GetUtcDay();
		State->PeriodicSubmitted = FirstPhaseBudget + SecondPhaseBudget;
		State->SummarySubmitted = SummaryBudget;
	}
	State->Version = UGameplayAnalyticsSaveGame::CurrentVersion;
	State->PeriodicSubmitted = FMath::Clamp(State->PeriodicSubmitted, 0, FirstPhaseBudget + SecondPhaseBudget);
	State->SummarySubmitted = FMath::Clamp(State->SummarySubmitted, 0, SummaryBudget);
	const auto SanitizeSeconds = [](double Value)
	{
		return FMath::IsFinite(Value) ? FMath::Clamp(Value, 0.0, GameplayAnalytics::MaxPersistedDurationSeconds) : 0.0;
	};
	State->ActiveGameplaySeconds = SanitizeSeconds(State->ActiveGameplaySeconds);
	State->PendingSessionSeconds = SanitizeSeconds(State->PendingSessionSeconds);
	State->OpenSessionSeconds = SanitizeSeconds(State->OpenSessionSeconds);
	State->IntervalSeconds = FMath::Fmod(SanitizeSeconds(State->IntervalSeconds), IntervalSeconds);
	int32 RetainedMetricCount = 0;
	for (auto MetricIterator = State->Metrics.CreateIterator(); MetricIterator; ++MetricIterator)
	{
		FGameplayAnalyticsMetric& Metric = MetricIterator.Value();
		if (bLegacy && MetricIterator.Key().StartsWith(TEXT("Performance:")) && Metric.Aggregation == EGameplayAnalyticsAggregation::Sum)
		{
			// Version 1 retained both sample count and sum, so migrate without losing samples.
			Metric.Aggregation = EGameplayAnalyticsAggregation::Average;
		}
		const bool bInvalidIdentifier = !IsEventIdValid(MetricIterator.Key())
			|| MetricIterator.Key() == TEXT("Session:GameplayDuration");
		const bool bInvalidAggregate =
			static_cast<uint8>(Metric.Aggregation) > static_cast<uint8>(EGameplayAnalyticsAggregation::Latest)
			|| Metric.Count < 0
			|| !FMath::IsFinite(Metric.Sum)
			|| !FMath::IsFinite(Metric.Latest)
			|| !FMath::IsFinite(Metric.LastSubmittedGameplaySeconds);
		if (bInvalidIdentifier || bInvalidAggregate || RetainedMetricCount >= MaxMetricKeys)
		{
			MetricIterator.RemoveCurrent();
			continue;
		}
		Metric.Priority = FMath::Clamp(Metric.Priority, -GameplayAnalytics::MaxPriority, GameplayAnalytics::MaxPriority);
		Metric.LastSubmittedGameplaySeconds = FMath::Clamp(Metric.LastSubmittedGameplaySeconds, 0.0, State->ActiveGameplaySeconds);
		++RetainedMetricCount;
	}
}

bool UGameplayAnalyticsSubsystem::IsEventIdValid(const FString& EventId) const
{
	if (EventId.IsEmpty() || EventId.Len() > GameplayAnalytics::MaxEventIdLength)
	{
		return false;
	}
	TArray<FString> Parts;
	EventId.ParseIntoArray(Parts, TEXT(":"), false);
	if (Parts.Num() < 1 || Parts.Num() > GameplayAnalytics::MaxEventSegments)
	{
		return false;
	}
	for (const FString& Part : Parts)
	{
		if (Part.IsEmpty() || Part.Len() > GameplayAnalytics::MaxEventSegmentLength)
		{
			return false;
		}
		for (const TCHAR Character : Part)
		{
			const bool bAsciiAlnum =
				(Character >= 'a' && Character <= 'z') || (Character >= 'A' && Character <= 'Z') || (Character >= '0' && Character <= '9');
			if (!bAsciiAlnum && Character != '_' && Character != '-' && Character != '.')
			{
				return false;
			}
		}
	}
	return true;
}

bool UGameplayAnalyticsSubsystem::RegisterMetric(const FString& EventId, EGameplayAnalyticsAggregation Aggregation, int32 Priority)
{
	if (!State || !IsEventIdValid(EventId)
		|| EventId == TEXT("Session:GameplayDuration")
		|| static_cast<uint8>(Aggregation) > static_cast<uint8>(EGameplayAnalyticsAggregation::Latest))
	{
		return false;
	}
	if (const FGameplayAnalyticsMetric* Existing = State->Metrics.Find(EventId))
	{
		return Existing->Aggregation == Aggregation;
	}
	if (State->Metrics.Num() >= MaxMetricKeys)
	{
		return false;
	}
	FGameplayAnalyticsMetric Metric;
	Metric.Aggregation = Aggregation;
	Metric.Priority = FMath::Clamp(Priority, -GameplayAnalytics::MaxPriority, GameplayAnalytics::MaxPriority);
	// New identifiers do not get an artificial eight-hour age advantage.
	Metric.LastSubmittedGameplaySeconds = State->ActiveGameplaySeconds;
	State->Metrics.Add(EventId, Metric);
	bDirty = true;
	return true;
}

void UGameplayAnalyticsSubsystem::RecordEvent(const FString& EventId)
{
	if (!State)
	{
		return;
	}
	RefreshUtcDay();
	if (!State->Metrics.Contains(EventId) && !RegisterMetric(EventId, EGameplayAnalyticsAggregation::Count))
	{
		return;
	}
	FGameplayAnalyticsMetric& Metric = State->Metrics.FindChecked(EventId);
	if (Metric.Aggregation != EGameplayAnalyticsAggregation::Count)
	{
		return;
	}
	if (Metric.Count < MAX_int64)
	{
		++Metric.Count;
	}
	bDirty = true;
}

void UGameplayAnalyticsSubsystem::RecordValue(const FString& EventId, float Value)
{
	if (!State || !FMath::IsFinite(Value))
	{
		return;
	}
	RefreshUtcDay();
	if (!State->Metrics.Contains(EventId) && !RegisterMetric(EventId, EGameplayAnalyticsAggregation::Sum))
	{
		return;
	}
	FGameplayAnalyticsMetric& Metric = State->Metrics.FindChecked(EventId);
	if (Metric.Aggregation == EGameplayAnalyticsAggregation::Count)
	{
		return;
	}
	// Keep pending storage fixed-size even if the provider remains unavailable indefinitely.
	if (Metric.Count == MAX_int64)
	{
		return;
	}
	++Metric.Count;
	Metric.Sum += static_cast<double>(Value);
	Metric.Latest = Value;
	bDirty = true;
}

void UGameplayAnalyticsSubsystem::RecordPerformanceValue(const FString& EventId, float Value)
{
	// Load durations represent a mean in milliseconds, not the sum of repeated loads.
	if (RegisterMetric(EventId, EGameplayAnalyticsAggregation::Average))
	{
		RecordValue(EventId, Value);
	}
}

bool UGameplayAnalyticsSubsystem::IsActiveGameplayWorld(const UWorld* World) const
{
	if (!World || !World->IsGameWorld() || World->GetGameInstance() != GetGameInstance() || World != GetGameInstance()->GetWorld()
		|| World->IsPaused() || World->GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}
	const FString Map = UWorld::RemovePIEPrefix(FPackageName::GetShortName(World->GetMapName()));
	return !Map.Contains(TEXT("MainMenu")) && Map != TEXT("Entry");
}

void UGameplayAnalyticsSubsystem::OnWorldPostActorTick(UWorld* World, ELevelTick TickType, float DeltaSeconds)
{
	if (!IsActiveGameplayWorld(World) || TickType != LEVELTICK_All)
	{
		return;
	}
	// Engine frame time is undilated; world DeltaSeconds changes with gameplay time dilation.
	AdvanceGameplayTime(FApp::GetDeltaTime());
}

void UGameplayAnalyticsSubsystem::AdvanceGameplayTime(double Seconds)
{
	if (!State || !FMath::IsFinite(Seconds) || Seconds <= 0.0)
	{
		return;
	}
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
	if (bDirty && CheckpointSeconds >= GameplayAnalytics::CheckpointIntervalSeconds)
	{
		CheckpointSeconds = 0.0;
		RequestCheckpoint();
	}
}

void UGameplayAnalyticsSubsystem::OnPreLoadMap(const FWorldContext& Context, const FString& MapName)
{
	if (Context.OwningGameInstance != GetGameInstance())
	{
		return;
	}
	const FString Map = FPackageName::GetShortName(MapName);
	if (Map.Contains(TEXT("MainMenu")) || Map == TEXT("Entry"))
	{
		EndGameplaySession();
	}
	else
	{
		RequestCheckpoint();
	}
}

void UGameplayAnalyticsSubsystem::RefreshUtcDay()
{
	if (!State)
	{
		return;
	}
	const FString Today = GetUtcDay();
	// A backwards wall-clock adjustment must not replenish a consumed day.
	if (!State->UtcDay.IsEmpty() && State->UtcDay >= Today)
	{
		return;
	}
	State->UtcDay = Today;
	State->PeriodicSubmitted = 0;
	State->SummarySubmitted = 0;
	State->ActiveGameplaySeconds = 0.0;
	State->IntervalSeconds = 0.0;
	for (TPair<FString, FGameplayAnalyticsMetric>& Pair : State->Metrics)
	{
		Pair.Value.LastSubmittedGameplaySeconds = 0.0;
	}
	bDirty = true;
	RequestCheckpoint();
}

int32 UGameplayAnalyticsSubsystem::EarnedPeriodicAllowance() const
{
	if (!State)
	{
		return 0;
	}
	const double Active = State->ActiveGameplaySeconds;
	const int32 FirstIntervals = FMath::Clamp(
		FMath::FloorToInt(FMath::Min(Active, FirstPhaseSeconds) / IntervalSeconds), 0, GameplayAnalytics::ReportingIntervalsPerPhase);
	const int32 LaterIntervals =
		FMath::Clamp(FMath::FloorToInt(FMath::Max(0.0, FMath::Min(Active, EndPhaseSeconds) - FirstPhaseSeconds) / IntervalSeconds),
			0,
			GameplayAnalytics::ReportingIntervalsPerPhase);
	return (FirstIntervals * FirstPhaseBudget) / GameplayAnalytics::ReportingIntervalsPerPhase
		+ (LaterIntervals * SecondPhaseBudget) / GameplayAnalytics::ReportingIntervalsPerPhase;
}

bool UGameplayAnalyticsSubsystem::CanSubmit() const
{
#if WITH_DEV_AUTOMATION_TESTS
	if (bTestProviderUnavailable)
	{
		return false;
	}
	if (TestSink)
	{
		return true;
	}
#endif
#if !WITH_EDITOR
	const UScenePerformanceAnalyticsSubsystem* SceneAnalytics = GetGameInstance()->GetSubsystem<UScenePerformanceAnalyticsSubsystem>();
	return SceneAnalytics && SceneAnalytics->IsAnalyticsReady() && UGameAnalytics::GetInstance();
#else
	return false;
#endif
}

bool UGameplayAnalyticsSubsystem::SubmitValue(const FString& Id, double Value) const
{
#if WITH_DEV_AUTOMATION_TESTS
	if (TestSink)
	{
		return TestSink(Id, Value);
	}
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

double UGameplayAnalyticsSubsystem::GetAggregatedValue(const FGameplayAnalyticsMetric& Metric)
{
	switch (Metric.Aggregation)
	{
		case EGameplayAnalyticsAggregation::Count:
			return static_cast<double>(Metric.Count);
		case EGameplayAnalyticsAggregation::Sum:
			return Metric.Sum;
		case EGameplayAnalyticsAggregation::Average:
			return Metric.Sum / Metric.Count;
		case EGameplayAnalyticsAggregation::Latest:
			return Metric.Latest;
	}
	return 0.0;
}

bool UGameplayAnalyticsSubsystem::TryReserveSummary()
{
	if (bSummaryAttemptedInFlush || State->PendingSessionSeconds <= 0.0 || State->SummarySubmitted >= SummaryBudget)
	{
		return false;
	}
	bSummaryAttemptedInFlush = true;
	Reservation.EventId = TEXT("Session:GameplayDuration");
	Reservation.UtcDay = State->UtcDay;
	Reservation.Value = State->PendingSessionSeconds;
	Reservation.bSummary = true;
	State->PendingSessionSeconds = 0.0;
	++State->SummarySubmitted;
	bDirty = true;
	return true;
}

bool UGameplayAnalyticsSubsystem::TryReservePeriodicMetric()
{
	if (State->ActiveGameplaySeconds > EndPhaseSeconds || State->PeriodicSubmitted >= EarnedPeriodicAllowance())
	{
		return false;
	}

	FString SelectedId;
	double SelectedValue = 0.0;
	double HighestScore = -MAX_dbl;
	for (const TPair<FString, FGameplayAnalyticsMetric>& Pair : State->Metrics)
	{
		if (Pair.Value.Count <= 0 || FlushMetricIds.Contains(Pair.Key))
		{
			continue;
		}
		const double Value = GetAggregatedValue(Pair.Value);
		if (!FMath::IsFinite(Value) || FMath::Abs(Value) > MAX_flt)
		{
			continue;
		}
		const double WaitSeconds = FMath::Max(0.0, State->ActiveGameplaySeconds - Pair.Value.LastSubmittedGameplaySeconds);
		const double Score = WaitSeconds + Pair.Value.Priority * GameplayAnalytics::PriorityAgeBonusSeconds;
		if (SelectedId.IsEmpty() || Score > HighestScore || (Score == HighestScore && Pair.Key < SelectedId))
		{
			SelectedId = Pair.Key;
			SelectedValue = Value;
			HighestScore = Score;
		}
	}
	if (SelectedId.IsEmpty())
	{
		return false;
	}

	Reservation.EventId = SelectedId;
	Reservation.UtcDay = State->UtcDay;
	Reservation.Value = SelectedValue;
	Reservation.Metric = State->Metrics.FindChecked(SelectedId);
	FGameplayAnalyticsMetric& PendingMetric = State->Metrics.FindChecked(SelectedId);
	PendingMetric.Count = 0;
	PendingMetric.Sum = 0.0;
	PendingMetric.Latest = 0.0;
	PendingMetric.LastSubmittedGameplaySeconds = State->ActiveGameplaySeconds;
	++State->PeriodicSubmitted;
	FlushMetricIds.Add(SelectedId);
	bDirty = true;
	return true;
}

bool UGameplayAnalyticsSubsystem::TryReserveSubmission()
{
	if (!CanSubmit())
	{
		return false;
	}
	return TryReserveSummary() || TryReservePeriodicMetric();
}

void UGameplayAnalyticsSubsystem::RestoreReservation(const FSubmissionReservation& SavedReservation)
{
	const bool bSameDay = SavedReservation.UtcDay == State->UtcDay;
	if (SavedReservation.bSummary)
	{
		State->PendingSessionSeconds += SavedReservation.Value;
		if (bSameDay)
		{
			--State->SummarySubmitted;
		}
	}
	else
	{
		FGameplayAnalyticsMetric& PendingMetric = State->Metrics.FindChecked(SavedReservation.EventId);
		if (PendingMetric.Count == 0)
		{
			PendingMetric.Latest = SavedReservation.Metric.Latest;
		}
		PendingMetric.Count = SavedReservation.Metric.Count > MAX_int64 - PendingMetric.Count
			? MAX_int64 : PendingMetric.Count + SavedReservation.Metric.Count;
		PendingMetric.Sum += SavedReservation.Metric.Sum;
		PendingMetric.LastSubmittedGameplaySeconds = bSameDay ? SavedReservation.Metric.LastSubmittedGameplaySeconds : 0.0;
		if (bSameDay)
		{
			--State->PeriodicSubmitted;
		}
	}
	bDirty = true;
	RequestCheckpoint();
}

void UGameplayAnalyticsSubsystem::RequestCheckpoint()
{
	bSaveRequested |= bDirty;
}

void UGameplayAnalyticsSubsystem::BeginAsyncSave()
{
	check(!SaveFuture.IsValid());
	TArray<uint8> SaveBytes;
#if WITH_DEV_AUTOMATION_TESTS
	if (!TestPersist || TestAsyncSave)
#endif
	{
		if (!UGameplayStatics::SaveGameToMemory(State, SaveBytes))
		{
			CompleteAsyncSave(false);
			return;
		}
	}
	// Serialization stays on the game thread. The worker owns immutable bytes and never touches UObjects.
	bDirty = false;
	bSaveRequested = false;
	CheckpointSeconds = 0.0;
#if WITH_DEV_AUTOMATION_TESTS
	if (TestAsyncSave)
	{
		SaveFuture = TestAsyncSave(MoveTemp(SaveBytes));
		return;
	}
	if (TestPersist)
	{
		TPromise<bool> SavePromise;
		SaveFuture = SavePromise.GetFuture();
		SavePromise.SetValue(TestPersist());
		return;
	}
#endif
	// Resolve the platform backend on the game thread before handing immutable data to the worker.
	ISaveGameSystem* const SaveSystem = IPlatformFeaturesModule::Get().GetSaveGameSystem();
	if (!SaveSystem)
	{
		CompleteAsyncSave(false);
		return;
	}
	const FString TargetSlot = SlotName;
	SaveFuture = Async(EAsyncExecution::ThreadPool,
		[SaveSystem, TargetSlot, SaveBytes = MoveTemp(SaveBytes)]()
		{
			return SaveSystem->SaveGame(false, *TargetSlot, 0, SaveBytes);
		});
}

void UGameplayAnalyticsSubsystem::CompleteAsyncSave(bool bSuccess)
{
	RefreshUtcDay();
	FSubmissionReservation SavedReservation = MoveTemp(Reservation);
	Reservation = FSubmissionReservation();
	if (!SavedReservation.EventId.IsEmpty())
	{
		const bool bSameDay = SavedReservation.UtcDay == State->UtcDay;
		const bool bWithinActiveTime = SavedReservation.bSummary || State->ActiveGameplaySeconds <= EndPhaseSeconds;
		if (!bSuccess || bShuttingDown || !bSameDay || !bWithinActiveTime || !CanSubmit()
			|| !SubmitValue(SavedReservation.EventId, SavedReservation.Value))
		{
			RestoreReservation(SavedReservation);
			// An old-day reservation is retried using the new day's allowance, never its old quota.
			if (bSameDay)
			{
				bFlushRequested = false;
			}
			else
			{
				bSummaryAttemptedInFlush = false;
			}
		}
	}
	if (!bSuccess)
	{
		bDirty = true;
		RequestCheckpoint();
		SaveRetrySeconds = GameplayAnalytics::CheckpointIntervalSeconds;
		UE_LOG(LogGameplayAnalytics, Warning, TEXT("Analytics checkpoint failed; retry deferred for one minute"));
	}
}

bool UGameplayAnalyticsSubsystem::TickPersistence(float DeltaSeconds)
{
	if (bShuttingDown || !State)
	{
		return false;
	}
	uint64 CurrentFrame = GFrameCounter;
#if WITH_DEV_AUTOMATION_TESTS
	if (TestFrame.IsSet())
	{
		CurrentFrame = TestFrame.GetValue();
	}
#endif
	if (CurrentFrame == LastPersistenceFrame)
	{
		return true;
	}
	LastPersistenceFrame = CurrentFrame;
	if (SaveFuture.IsValid())
	{
		if (SaveFuture.IsReady())
		{
			const bool bSuccess = SaveFuture.Get();
			SaveFuture = TFuture<bool>();
			CompleteAsyncSave(bSuccess);
		}
		// Complete at most one operation per frame; never start the next save in this callback.
		return true;
	}
	SaveRetrySeconds = FMath::Max(0.0, SaveRetrySeconds - FMath::Max(0.0f, DeltaSeconds));
	if (SaveRetrySeconds > 0.0)
	{
		return true;
	}
	RefreshUtcDay();
	if (bFlushRequested)
	{
		if (TryReserveSubmission())
		{
			BeginAsyncSave();
			return true;
		}
		bFlushRequested = false;
		RequestCheckpoint();
	}
	if (bSaveRequested)
	{
		bSaveRequested = false;
		if (bDirty)
		{
			BeginAsyncSave();
		}
	}
	return true;
}

void UGameplayAnalyticsSubsystem::FlushAnalytics()
{
	if (!State || bShuttingDown)
	{
		return;
	}
	RefreshUtcDay();
	if (!bFlushRequested)
	{
		FlushMetricIds.Empty();
		bSummaryAttemptedInFlush = false;
	}
	bFlushRequested = true;
	RequestCheckpoint();
}

void UGameplayAnalyticsSubsystem::EndGameplaySession()
{
	if (!State)
	{
		return;
	}
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

bool UGameplayAnalyticsSubsystem::PersistSynchronously()
{
	check(!SaveFuture.IsValid());
#if WITH_DEV_AUTOMATION_TESTS
	if (TestPersist)
	{
		return TestPersist();
	}
#endif
	const bool bSuccess = UGameplayStatics::SaveGameToSlot(State, SlotName, 0);
	if (!bSuccess)
	{
		UE_LOG(LogGameplayAnalytics, Warning, TEXT("Final analytics checkpoint failed"));
	}
	return bSuccess;
}

void UGameplayAnalyticsSubsystem::FinishPersistence()
{
	// Shutdown is the only blocking path: joining our worker does not require a game-thread callback.
	if (SaveFuture.IsValid())
	{
		const bool bSuccess = SaveFuture.Get();
		SaveFuture = TFuture<bool>();
		CompleteAsyncSave(bSuccess);
	}
	EndGameplaySession();
	bSummaryAttemptedInFlush = false;
	if (CanSubmit() && TryReserveSummary())
	{
		const FSubmissionReservation SummaryReservation = MoveTemp(Reservation);
		Reservation = FSubmissionReservation();
		if (PersistSynchronously())
		{
			bDirty = false;
			RefreshUtcDay();
			if (State->UtcDay != SummaryReservation.UtcDay || !SubmitValue(SummaryReservation.EventId, SummaryReservation.Value))
			{
				RestoreReservation(SummaryReservation);
			}
		}
		else
		{
			RestoreReservation(SummaryReservation);
		}
	}
	if (bDirty)
	{
		PersistSynchronously();
	}
	bFlushRequested = false;
	bSaveRequested = false;
}
