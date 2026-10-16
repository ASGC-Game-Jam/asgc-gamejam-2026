// Copyright (c) 2026 ASGC
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayAnalyticsSubsystem.generated.h"

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
	UPROPERTY(SaveGame)
	int32 Version = 1;
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

	/** Register an identifier once; duplicate registration with a different strategy is rejected. */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|Analytics")
	bool RegisterMetric(const FString& EventId, EGameplayAnalyticsAggregation Aggregation, int32 Priority = 0);

	UFUNCTION(BlueprintCallable, Category = "Atlantis|Analytics")
	void RecordEvent(const FString& EventId);

	UFUNCTION(BlueprintCallable, Category = "Atlantis|Analytics")
	void RecordValue(const FString& EventId, float Value);

	/** Submit only within the earned shared budget. Safe to call manually. */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|Analytics")
	void FlushAnalytics();

	/** Use on normal departure from gameplay. Idempotent until gameplay resumes. */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|Analytics")
	void EndGameplaySession();

	/** Shared path for existing scene-performance events. Counts toward the periodic budget. */
	void RecordPerformanceValue(const FString& EventId, float Value);

private:
	void OnWorldPostActorTick(UWorld* World, ELevelTick TickType, float DeltaSeconds);
	void OnPreLoadMap(const FWorldContext& Context, const FString& MapName);
	bool IsActiveGameplayWorld(const UWorld* World) const;
	bool IsEventIdValid(const FString& EventId) const;
	void RefreshUtcDay();
	void Persist() const;
	void SubmitSessionSummary();
	int32 EarnedPeriodicAllowance() const;
	bool SubmitValue(const FString& Id, double Value) const;
	void FlushPendingWithBudget(int32 Available);

	FDelegateHandle TickHandle;
	FDelegateHandle PreLoadHandle;
	UPROPERTY()
	TObjectPtr<UGameplayAnalyticsSaveGame> State;
	double IntervalAccumulatorSeconds = 0.0;
	double CurrentSessionSeconds = 0.0;
	bool bSessionOpen = false;
	bool bInitialized = false;

	static constexpr int32 FirstPhaseBudget = 320;
	static constexpr int32 SecondPhaseBudget = 150;
	static constexpr int32 SummaryBudget = 10;
	static constexpr double IntervalSeconds = 300.0;
	static constexpr double FirstPhaseSeconds = 14400.0;
	static constexpr double EndPhaseSeconds = 28800.0;
	static constexpr int32 MaxMetricKeys = 1024;
	static constexpr TCHAR SaveSlot[] = TEXT("GameplayAnalyticsBudget");
};
