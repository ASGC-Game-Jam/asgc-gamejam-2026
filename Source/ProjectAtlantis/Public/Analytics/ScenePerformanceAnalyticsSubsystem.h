#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ScenePerformanceAnalyticsSubsystem.generated.h"

class UWorld;
struct FWorldContext;

/**
 * Automatically tracks map loading, scene readiness, scene transitions, and initial startup timing.
 * The subsystem is created once per GameInstance and requires no per-level setup.
 */
UCLASS()
class PROJECTATLANTIS_API UScenePerformanceAnalyticsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	void OnPreLoadMap(const FWorldContext& WorldContext, const FString& MapName);
	void OnPostLoadMap(UWorld* LoadedWorld);
	void OnWorldPostActorTick(UWorld* World, const ELevelTick TickType, const float DeltaSeconds);

	bool IsTrackedWorld(const UWorld* World) const;
	void SubmitTiming(const FString& EventId, const float Milliseconds) const;

	static FString GetSceneName(const UWorld* World);
	static FString GetSceneName(const FString& MapName);
	static FString SanitizeEventSegment(const FString& Segment);

	FDelegateHandle PreLoadMapHandle;
	FDelegateHandle PostLoadMapHandle;
	FDelegateHandle WorldPostActorTickHandle;

	double GameInstanceStartSeconds = 0.0;
	double LoadStartSeconds = 0.0;
	double LoadCompleteSeconds = 0.0;

	FString CurrentSceneName;
	FString FromSceneName;
	FString ToSceneName;

	TWeakObjectPtr<UWorld> PendingWorld;

	bool bLoadInProgress = false;
	bool bWaitingForFirstTick = false;
	bool bStartupReported = false;
};
