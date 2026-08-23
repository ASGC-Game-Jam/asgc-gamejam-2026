#include "Analytics/ScenePerformanceAnalyticsSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameAnalytics.h"
#include "Analytics.h"
#include "Interfaces/IAnalyticsProvider.h"
#include "HAL/PlatformTime.h"
#include "Misc/PackageName.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogScenePerformanceAnalytics, Log, All);

void UScenePerformanceAnalyticsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	GameInstanceStartSeconds = FPlatformTime::Seconds();

#if !WITH_EDITOR

	if (TSharedPtr<IAnalyticsProvider> AnalyticsProvider = FAnalytics::Get().GetDefaultConfiguredProvider())
	{
		if (AnalyticsProvider->StartSession())
		{
			UE_LOG(LogScenePerformanceAnalytics, Display, TEXT("GameAnalytics session started"));
		}
		else
		{
			UE_LOG(LogScenePerformanceAnalytics, Warning, TEXT("GameAnalytics session failed to start"));
		}
	}
	else
	{
		UE_LOG(LogScenePerformanceAnalytics, Warning, TEXT("GameAnalytics provider unavailable"));
	}
#endif

	PreLoadMapHandle = FCoreUObjectDelegates::PreLoadMapWithContext.AddUObject (this, &UScenePerformanceAnalyticsSubsystem::HandlePreLoadMap);
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject (this, &UScenePerformanceAnalyticsSubsystem::HandlePostLoadMap);
	WorldPostActorTickHandle = FWorldDelegates::OnWorldPostActorTick.AddUObject(this, &UScenePerformanceAnalyticsSubsystem::HandleWorldPostActorTick);

	if (const UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
		CurrentSceneName = GetSceneName(World);

}

void UScenePerformanceAnalyticsSubsystem::Deinitialize()
{
	if (PreLoadMapHandle.IsValid())
		FCoreUObjectDelegates::PreLoadMapWithContext.Remove(PreLoadMapHandle);

	if (PostLoadMapHandle.IsValid())
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);

	if (WorldPostActorTickHandle.IsValid())
		FWorldDelegates::OnWorldPostActorTick.Remove(WorldPostActorTickHandle);

	Super::Deinitialize();
}

void UScenePerformanceAnalyticsSubsystem::HandlePreLoadMap(const FWorldContext& WorldContext, const FString& MapName)
{
	if (WorldContext.OwningGameInstance != GetGameInstance())
		return;

	const UWorld* CurrentWorld = WorldContext.World();
	FromSceneName = GetSceneName(CurrentWorld);

	if (!bStartupReported)
		FromSceneName = TEXT("Startup");
	else if (FromSceneName.IsEmpty())
		FromSceneName = CurrentSceneName;

	ToSceneName = GetSceneName(MapName);
	LoadStartSeconds = FPlatformTime::Seconds();
	LoadCompleteSeconds = 0.0;
	PendingWorld.Reset();
	bLoadInProgress = true;
	bWaitingForFirstTick = false;

	UE_LOG(LogScenePerformanceAnalytics, Display, TEXT("Scene load started: %s -> %s"), FromSceneName.IsEmpty() ? TEXT("Startup") : *FromSceneName, *ToSceneName);
}

void UScenePerformanceAnalyticsSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	if (!IsTrackedWorld(LoadedWorld))
		return;

	CurrentSceneName = GetSceneName(LoadedWorld);

	if (!bLoadInProgress)
		return;

	LoadCompleteSeconds = FPlatformTime::Seconds();
	PendingWorld = LoadedWorld;
	bWaitingForFirstTick = true;
}

void UScenePerformanceAnalyticsSubsystem::HandleWorldPostActorTick(UWorld* World, ELevelTick TickType, float DeltaSeconds)
{
	if (!IsTrackedWorld(World))
		return;

	const double NowSeconds = FPlatformTime::Seconds();
	const FString SceneName = GetSceneName(World);

	if (!bStartupReported)
	{
		bStartupReported = true;
		const float StartupMilliseconds = static_cast<float>((NowSeconds - GameInstanceStartSeconds) * 1000.0);
		SubmitTiming(FString::Printf(TEXT("Performance:Startup:%s"), *SceneName), StartupMilliseconds);
		UE_LOG(LogScenePerformanceAnalytics, Display, TEXT("Initial scene ready: %s | startup %.2f ms"), *SceneName, StartupMilliseconds);
	}

	if (!bLoadInProgress || !bWaitingForFirstTick || PendingWorld.Get() != World)
		return;

	const FString LoadedSceneName = ToSceneName.IsEmpty() ? SceneName : ToSceneName;
	const float MapLoadMilliseconds = static_cast<float>((LoadCompleteSeconds - LoadStartSeconds) * 1000.0);
	const float SceneReadyMilliseconds = static_cast<float>((NowSeconds - LoadStartSeconds) * 1000.0);
	SubmitTiming(FString::Printf(TEXT("Performance:MapLoad:%s"), *LoadedSceneName), MapLoadMilliseconds);
	SubmitTiming(FString::Printf(TEXT("Performance:SceneReady:%s"), *LoadedSceneName), SceneReadyMilliseconds);

	if (!FromSceneName.IsEmpty())
		SubmitTiming(FString::Printf(TEXT("Performance:Transition:%s:%s"), *FromSceneName, *LoadedSceneName), SceneReadyMilliseconds);

	UE_LOG(LogScenePerformanceAnalytics, Display, TEXT("Scene ready: %s -> %s | map load %.2f ms | ready %.2f ms"),
		FromSceneName.IsEmpty() ? TEXT("Startup") : *FromSceneName,	*LoadedSceneName, MapLoadMilliseconds, SceneReadyMilliseconds);

	CurrentSceneName = LoadedSceneName;
	FromSceneName.Reset();
	ToSceneName.Reset();
	PendingWorld.Reset();
	bLoadInProgress = false;
	bWaitingForFirstTick = false;
	LoadStartSeconds = 0.0;
	LoadCompleteSeconds = 0.0;
}

bool UScenePerformanceAnalyticsSubsystem::IsTrackedWorld(const UWorld* World) const
{
	return World && World->IsGameWorld() && World->GetGameInstance() == GetGameInstance();
}

void UScenePerformanceAnalyticsSubsystem::SubmitTiming(const FString& EventId, float Milliseconds) const
{
	if (EventId.IsEmpty() || Milliseconds < 0.0f)
		return;

#if !WITH_EDITOR

	if (UGameAnalytics* GameAnalytics = UGameAnalytics::GetInstance())
		GameAnalytics->AddDesignEventWithValue(EventId, Milliseconds);
#endif
}

FString UScenePerformanceAnalyticsSubsystem::GetSceneName(const UWorld* World)
{
	return World ? GetSceneName(World->GetMapName()) : FString();
}

FString UScenePerformanceAnalyticsSubsystem::GetSceneName(const FString& MapName)
{
	if (MapName.IsEmpty())
		return FString();

	FString ShortName = FPackageName::GetShortName(MapName);
	ShortName = UWorld::RemovePIEPrefix(ShortName, nullptr);
	return SanitizeEventSegment(ShortName);
}

FString UScenePerformanceAnalyticsSubsystem::SanitizeEventSegment(const FString& Segment)
{
	FString Sanitized;
	Sanitized.Reserve(Segment.Len());

	for (const TCHAR Character : Segment)
	{
		const bool bAllowed = FChar::IsAlnum(Character)
			|| Character == TEXT('_')
			|| Character == TEXT('-')
			|| Character == TEXT('.')
			|| Character == TEXT(' ')
			|| Character == TEXT('(')
			|| Character == TEXT(')')
			|| Character == TEXT('!')
			|| Character == TEXT('?');

		Sanitized.AppendChar(bAllowed ? Character : TEXT('_'));
	}

	if (Sanitized.IsEmpty())
		Sanitized = TEXT("Unknown");

	// GameAnalytics limits each event hierarchy segment to 32 characters.
	// Left(32) truncates longer map/scene names so generated event IDs stay within that limit instead of relying on GameAnalytics to truncate them after submission.
	return Sanitized.Left(32);
}
