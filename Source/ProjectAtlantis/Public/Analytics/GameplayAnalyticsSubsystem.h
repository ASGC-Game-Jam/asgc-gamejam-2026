// Copyright (c) 2026 ASGC
#pragma once

#include "CoreMinimal.h"
#include "Async/Future.h"
#include "Containers/Ticker.h"
#include "Engine/EngineBaseTypes.h"
#include "GameFramework/SaveGame.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Templates/Function.h"
#include "GameplayAnalyticsSubsystem.generated.h"

class UWorld;
struct FWorldContext;

UENUM(BlueprintType)
enum class EGameplayAnalyticsAggregation : uint8
{
	Count,
	Sum,
	Average,
	Latest
};

USTRUCT(BlueprintType)
struct FGameplayAnalyticsMetric
{
	GENERATED_BODY()

	UPROPERTY(SaveGame)
	EGameplayAnalyticsAggregation Aggregation = EGameplayAnalyticsAggregation::Count;

	UPROPERTY(SaveGame)
	int32 Priority = 0;

	UPROPERTY(SaveGame)
	int64 Count = 0;

	UPROPERTY(SaveGame)
	double Sum = 0.0;

	UPROPERTY(SaveGame)
	double Latest = 0.0;

	UPROPERTY(SaveGame)
	double LastSubmittedGameplaySeconds = -1.0;
};

UCLASS()
class PROJECTATLANTIS_API UGameplayAnalyticsSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 CurrentVersion = 2;

	UPROPERTY(SaveGame)
	int32 Version = CurrentVersion;

	UPROPERTY(SaveGame)
	FString UtcDay;

	UPROPERTY(SaveGame)
	int32 PeriodicSubmitted = 0;

	UPROPERTY(SaveGame)
	int32 SummarySubmitted = 0;

	UPROPERTY(SaveGame)
	double ActiveGameplaySeconds = 0.0;

	UPROPERTY(SaveGame)
	double PendingSessionSeconds = 0.0;

	UPROPERTY(SaveGame)
	double OpenSessionSeconds = 0.0;

	UPROPERTY(SaveGame)
	double IntervalSeconds = 0.0;

	UPROPERTY(SaveGame)
	TMap<FString, FGameplayAnalyticsMetric> Metrics;
};

/**
 * Local gameplay analytics queue and shared daily GameAnalytics budget.
 * Producers never communicate with GameAnalytics directly.
 */
UCLASS()
class PROJECTATLANTIS_API UGameplayAnalyticsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Register a stable ID and aggregation strategy. Priority is clamped to [-100, 100]; higher values send sooner. */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|Analytics")
	bool RegisterMetric(const FString& EventId, EGameplayAnalyticsAggregation Aggregation, int32 Priority = 0);

	/** Add one occurrence to a Count metric without submitting it. Unregistered IDs default to Count. */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|Analytics")
	void RecordEvent(const FString& EventId);

	/** Aggregate a finite observation in producer-defined units. Unregistered IDs default to Sum. */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|Analytics")
	void RecordValue(const FString& EventId, float Value);

	/** Request deferred submissions within the earned budget. Repeated requests never increase the allowance. */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|Analytics")
	void FlushAnalytics();

	/** Use on normal departure from gameplay. Idempotent until gameplay resumes. */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|Analytics")
	void EndGameplaySession();

	/** Record a scene-performance sample in milliseconds using Average and the shared periodic budget. */
	void RecordPerformanceValue(const FString& EventId, float Value);

private:
	friend struct FGameplayAnalyticsTestAccess;

	struct FSubmissionReservation
	{
		FString EventId;
		FString UtcDay;
		FGameplayAnalyticsMetric Metric;
		double Value = 0.0;
		bool bSummary = false;
	};

	void AdvanceGameplayTime(double Seconds);
	void ValidateState();
	FString GetUtcDay() const;
	void OnWorldPostActorTick(UWorld* World, ELevelTick TickType, float DeltaSeconds);
	void OnPreLoadMap(const FWorldContext& Context, const FString& MapName);
	bool IsActiveGameplayWorld(const UWorld* World) const;
	bool IsEventIdValid(const FString& EventId) const;
	void RefreshUtcDay();
	void RequestCheckpoint();
	bool TickPersistence(float DeltaSeconds);
	void BeginAsyncSave();
	void CompleteAsyncSave(bool bSuccess);
	bool TryReserveSubmission();
	bool TryReserveSummary();
	bool TryReservePeriodicMetric();
	void RestoreReservation(const FSubmissionReservation& SavedReservation);
	void FinishPersistence();
	bool PersistSynchronously();
	static double GetAggregatedValue(const FGameplayAnalyticsMetric& Metric);
	int32 EarnedPeriodicAllowance() const;
	bool SubmitValue(const FString& Id, double Value) const;
	bool CanSubmit() const;

	FDelegateHandle TickHandle;
	FDelegateHandle PreLoadHandle;
	FTSTicker::FDelegateHandle PersistenceTickerHandle;
	UPROPERTY()
	TObjectPtr<UGameplayAnalyticsSaveGame> State;
	bool bSessionOpen = false;
	bool bDirty = false;
	bool bSaveRequested = false;
	bool bFlushRequested = false;
	bool bSummaryAttemptedInFlush = false;
	bool bShuttingDown = false;
	TFuture<bool> SaveFuture;
	FSubmissionReservation Reservation;
	TSet<FString> FlushMetricIds;
	uint64 LastPersistenceFrame = MAX_uint64;
	double SaveRetrySeconds = 0.0;
	double CheckpointSeconds = 0.0;
	FString SlotName = TEXT("GameplayAnalyticsBudget");
#if WITH_DEV_AUTOMATION_TESTS
	TFunction<bool(const FString&, double)> TestSink;
	TFunction<bool()> TestPersist;
	TFunction<TFuture<bool>(TArray<uint8>&&)> TestAsyncSave;
	FString TestUtcDay;
	TOptional<uint64> TestFrame;
	bool bTestProviderUnavailable = false;
#endif

	static constexpr int32 FirstPhaseBudget = 320;
	static constexpr int32 SecondPhaseBudget = 150;
	static constexpr int32 SummaryBudget = 10;
	static constexpr double IntervalSeconds = 300.0;
	static constexpr double FirstPhaseSeconds = 14400.0;
	static constexpr double EndPhaseSeconds = 28800.0;
	static constexpr int32 MaxMetricKeys = 1024;
};
