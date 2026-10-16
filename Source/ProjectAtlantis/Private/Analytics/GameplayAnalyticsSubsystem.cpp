// Copyright (c) 2026 ASGC
#include "Analytics/GameplayAnalyticsSubsystem.h"

#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameAnalytics.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/DateTime.h"
#include "Misc/PackageName.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogGameplayAnalytics, Log, All);

void UGameplayAnalyticsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	State = Cast<UGameplayAnalyticsSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlot, 0));
	if (!State || State->Version != 1)
	{
		State = NewObject<UGameplayAnalyticsSaveGame>(this);
	}
	RefreshUtcDay();
	TickHandle = FWorldDelegates::OnWorldPostActorTick.AddUObject(this, &UGameplayAnalyticsSubsystem::OnWorldPostActorTick);
	PreLoadHandle = FCoreUObjectDelegates::PreLoadMapWithContext.AddUObject(this, &UGameplayAnalyticsSubsystem::OnPreLoadMap);
	bInitialized = true;
}

void UGameplayAnalyticsSubsystem::Deinitialize()
{
	EndGameplaySession();
	if (TickHandle.IsValid())
	{
		FWorldDelegates::OnWorldPostActorTick.Remove(TickHandle);
	}
	if (PreLoadHandle.IsValid())
	{
		FCoreUObjectDelegates::PreLoadMapWithContext.Remove(PreLoadHandle);
	}
	Persist();
	Super::Deinitialize();
}

bool UGameplayAnalyticsSubsystem::IsEventIdValid(const FString& EventId) const
{
	if (EventId.IsEmpty() || EventId.Len() > 160)
	{
		return false;
	}
	TArray<FString> Parts;
	EventId.ParseIntoArray(Parts, TEXT(":"), false);
	if (Parts.Num() < 1 || Parts.Num() > 5)
	{
		return false;
	}
	for (const FString& Part : Parts)
	{
		if (Part.IsEmpty() || Part.Len() > 32)
		{
			return false;
		}
		for (TCHAR C : Part)
		{
			if (!FChar::IsAlnum(C) && C != TEXT('_') && C != TEXT('-') && C != TEXT('.'))
			{
				return false;
			}
		}
	}
	return true;
}

bool UGameplayAnalyticsSubsystem::RegisterMetric(const FString& EventId, EGameplayAnalyticsAggregation Aggregation, int32 Priority)
{
	if (!IsEventIdValid(EventId) || !State)
	{
		UE_LOG(LogGameplayAnalytics, Warning, TEXT("Invalid metric identifier: %s"), *EventId);
		return false;
	}
	if (FGameplayAnalyticsMetric* Existing = State->Metrics.Find(EventId))
	{
		return Existing->Aggregation == Aggregation;
	}
	if (State->Metrics.Num() >= MaxMetricKeys)
	{
		UE_LOG(LogGameplayAnalytics, Warning, TEXT("Analytics metric capacity reached"));
		return false;
	}
	FGameplayAnalyticsMetric Metric;
	Metric.Aggregation = Aggregation;
	Metric.Priority = FMath::Clamp(Priority, -100, 100);
	State->Metrics.Add(EventId, Metric);
	Persist();
	return true;
}

void UGameplayAnalyticsSubsystem::RecordEvent(const FString& EventId)
{
	if (!State)
	{
		return;
	}
	RefreshUtcDay();
	FGameplayAnalyticsMetric* Metric = State->Metrics.Find(EventId);
	if (!Metric)
	{
		if (!RegisterMetric(EventId, EGameplayAnalyticsAggregation::Count))
		{
			return;
		}
		Metric = State->Metrics.Find(EventId);
	}
	if (!Metric || Metric->Aggregation != EGameplayAnalyticsAggregation::Count)
	{
		return;
	}
	Metric->Count++;
}

void UGameplayAnalyticsSubsystem::RecordValue(const FString& EventId, float Value)
{
	if (!State || !FMath::IsFinite(Value))
	{
		return;
	}
	RefreshUtcDay();
	FGameplayAnalyticsMetric* Metric = State->Metrics.Find(EventId);
	if (!Metric)
	{
		if (!RegisterMetric(EventId, EGameplayAnalyticsAggregation::Sum))
		{
			return;
		}
		Metric = State->Metrics.Find(EventId);
	}
	if (!Metric || Metric->Aggregation == EGameplayAnalyticsAggregation::Count)
	{
		return;
	}
	Metric->Count++;
	Metric->Sum += Value;
	Metric->Latest = Value;
}

void UGameplayAnalyticsSubsystem::RecordPerformanceValue(const FString& EventId, float Value)
{
	RecordValue(EventId, Value);
}

bool UGameplayAnalyticsSubsystem::IsActiveGameplayWorld(const UWorld* World) const
{
	if (!World || !World->IsGameWorld() || World->GetGameInstance() != GetGameInstance() || World->IsPaused())
	{
		return false;
	}
	const FString Map = FPackageName::GetShortName(World->GetMapName());
	return !Map.Contains(TEXT("MainMenu")) && !Map.Contains(TEXT("Entry"));
}

void UGameplayAnalyticsSubsystem::OnWorldPostActorTick(UWorld* World, ELevelTick TickType, float DeltaSeconds)
{
	if (!IsActiveGameplayWorld(World) || DeltaSeconds <= 0.f)
	{
		return;
	}
	RefreshUtcDay();
	bSessionOpen = true;
	State->ActiveGameplaySeconds += DeltaSeconds;
	CurrentSessionSeconds += DeltaSeconds;
	IntervalAccumulatorSeconds += DeltaSeconds;
	if (IntervalAccumulatorSeconds >= IntervalSeconds)
	{
		IntervalAccumulatorSeconds = FMath::Fmod(IntervalAccumulatorSeconds, IntervalSeconds);
		FlushAnalytics();
		Persist();
	}
}

void UGameplayAnalyticsSubsystem::OnPreLoadMap(const FWorldContext& Context, const FString& MapName)
{
	if (Context.OwningGameInstance != GetGameInstance())
	{
		return;
	}
	// Map transitions close the current gameplay session; subsequent gameplay creates a new one.
	EndGameplaySession();
}

void UGameplayAnalyticsSubsystem::RefreshUtcDay()
{
	if (!State)
	{
		return;
	}
	const FString Today = FDateTime::UtcNow().ToString(TEXT("%Y-%m-%d"));
	if (State->UtcDay == Today)
	{
		return;
	}
	// Preserve aggregates, including those deferred after the prior day's budget was exhausted.
	State->UtcDay = Today;
	State->PeriodicSubmitted = 0;
	State->SummarySubmitted = 0;
	State->ActiveGameplaySeconds = 0.0;
	IntervalAccumulatorSeconds = 0.0;
	Persist();
}

int32 UGameplayAnalyticsSubsystem::EarnedPeriodicAllowance() const
{
	if (!State)
	{
		return 0;
	}
	const double Active = State->ActiveGameplaySeconds;
	const int32 FirstIntervals = FMath::Clamp(FMath::FloorToInt(FMath::Min(Active, FirstPhaseSeconds) / IntervalSeconds), 0, 48);
	const int32 LaterIntervals = FMath::Clamp(FMath::FloorToInt(FMath::Max(0.0, FMath::Min(Active, EndPhaseSeconds) - FirstPhaseSeconds) / IntervalSeconds), 0, 48);
	return (FirstIntervals * FirstPhaseBudget) / 48 + (LaterIntervals * SecondPhaseBudget) / 48;
}

bool UGameplayAnalyticsSubsystem::SubmitValue(const FString& Id, double Value) const
{
#if !WITH_EDITOR
	if (UGameAnalytics* GA = UGameAnalytics::GetInstance())
	{
		GA->AddDesignEventWithValue(Id, static_cast<float>(Value));
		return true; // SDK queued the event; this does not guarantee server delivery.
	}
#endif
	return false;
}

void UGameplayAnalyticsSubsystem::FlushPendingWithBudget(int32 Available)
{
	if (!State || Available <= 0)
	{
		return;
	}
	struct FCandidate
	{
		FString Id;
		double Score;
	};
	TArray<FCandidate> Candidates;
	for (const TPair<FString, FGameplayAnalyticsMetric>& Pair : State->Metrics)
	{
		if (Pair.Value.Count <= 0)
		{
			continue;
		}
		const double Wait = Pair.Value.LastSubmittedGameplaySeconds < 0.0
			? 28800.0 : FMath::Max(0.0, State->ActiveGameplaySeconds - Pair.Value.LastSubmittedGameplaySeconds);
		Candidates.Add({Pair.Key, Wait + (Pair.Value.Priority * 300.0)});
	}
	Candidates.Sort([](const FCandidate& A, const FCandidate& B)
	{
		return A.Score == B.Score ? A.Id < B.Id : A.Score > B.Score;
	});
	for (const FCandidate& Candidate : Candidates)
	{
		if (Available <= 0)
		{
			break;
		}
		FGameplayAnalyticsMetric* Metric = State->Metrics.Find(Candidate.Id);
		if (!Metric || Metric->Count <= 0)
		{
			continue;
		}
		double Value = 0.0;
		switch (Metric->Aggregation)
		{
			case EGameplayAnalyticsAggregation::Count: Value = static_cast<double>(Metric->Count); break;
			case EGameplayAnalyticsAggregation::Sum: Value = Metric->Sum; break;
			case EGameplayAnalyticsAggregation::Average: Value = Metric->Sum / Metric->Count; break;
			case EGameplayAnalyticsAggregation::Latest: Value = Metric->Latest; break;
		}
		if (!SubmitValue(Candidate.Id, Value))
		{
			break;
		}
		Metric->Count = 0;
		Metric->Sum = 0.0;
		Metric->Latest = 0.0;
		Metric->LastSubmittedGameplaySeconds = State->ActiveGameplaySeconds;
		++State->PeriodicSubmitted;
		--Available;
	}
	Persist();
}

void UGameplayAnalyticsSubsystem::FlushAnalytics()
{
	if (!State)
	{
		return;
	}
	RefreshUtcDay();
	if (State->ActiveGameplaySeconds > EndPhaseSeconds)
	{
		return;
	}
	const int32 Allowed = FMath::Min(FirstPhaseBudget + SecondPhaseBudget, EarnedPeriodicAllowance());
	FlushPendingWithBudget(FMath::Max(0, Allowed - State->PeriodicSubmitted));
}

void UGameplayAnalyticsSubsystem::SubmitSessionSummary()
{
	if (!State || State->PendingSessionSeconds <= 0.0 || State->SummarySubmitted >= SummaryBudget)
	{
		return;
	}
	if (SubmitValue(TEXT("Session:GameplayDuration"), State->PendingSessionSeconds))
	{
		State->PendingSessionSeconds = 0.0;
		++State->SummarySubmitted;
	}
}

void UGameplayAnalyticsSubsystem::EndGameplaySession()
{
	if (!State || !bSessionOpen)
	{
		return;
	}
	RefreshUtcDay();
	bSessionOpen = false;
	State->PendingSessionSeconds += CurrentSessionSeconds;
	CurrentSessionSeconds = 0.0;
	FlushAnalytics();
	SubmitSessionSummary();
	Persist();
}

void UGameplayAnalyticsSubsystem::Persist() const
{
	if (State)
	{
		UGameplayStatics::SaveGameToSlot(State, SaveSlot, 0);
	}
}
