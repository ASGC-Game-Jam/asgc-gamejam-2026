// Fill out your copyright notice in the Description page of Project Settings.

#include "Audio/AtlantisAudioSubsystem.h"

#include "Audio/AtlantisAudioDeveloperSettings.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MetasoundSource.h"
#include "MetasoundGeneratorHandle.h"


DEFINE_LOG_CATEGORY_STATIC(LogAtlantisAudioSubsystem, Log, All);

static TAutoConsoleVariable<int32> CVarTrackAtlantisAudioAxisSweep(
	TEXT("Atlantis.Audio.AxesSweepTest"),
	0,
	TEXT("Enables a slow 0 to 1 automated parameter sweep for debugging the music subsystem axes.\n")
	TEXT("0: Disabled (Default)\n")
	TEXT("1: Enabled"),
	ECVF_Cheat
);



void UAtlantisAudioSubsystem::Initialize(FSubsystemCollectionBase &Collection)
{
	Super::Initialize(Collection);

	bIsActive = UAtlantisAudioDeveloperSettings::DetermineIfMusicSubsystemEnabled();
	
	UE_LOG(LogAtlantisAudioSubsystem, Log, TEXT("=================================================="));
	UE_LOG(LogAtlantisAudioSubsystem, Log, TEXT("Atlantis Audio Subsystem initialized. Status: [%s]"), bIsActive ? TEXT("ENABLED") : TEXT("BYPASSED"));
	UE_LOG(LogAtlantisAudioSubsystem, Log, TEXT("=================================================="));

	PostWorldInitHandle = FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &UAtlantisAudioSubsystem::HandlePostWorldInitialization);
}

void UAtlantisAudioSubsystem::Deinitialize()
{
	UE_LOG(LogAtlantisAudioSubsystem, Log, TEXT("Atlantis Audio Subsystem shutting down. Cleaning up subscriptions."));

	if (PostWorldInitHandle.IsValid())
	{
		FWorldDelegates::OnPostWorldInitialization.Remove(PostWorldInitHandle);
	}

	if (TrackedWorld.IsValid())
	{
		TrackedWorld->OnWorldBeginPlay.Remove(WorldBeginPlayHandle);
	}

	if (WorldCleanupHandle.IsValid())
	{
		FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
	}

	if (MusicAudioComponent)
	{
		UE_LOG(LogAtlantisAudioSubsystem, Log, TEXT("Deinitialize forced music component stop."));
		MusicAudioComponent->Stop();
		MusicAudioComponent = nullptr;
	}

	GeneratorHandle = nullptr;

	Super::Deinitialize();
}

void UAtlantisAudioSubsystem::HandlePostWorldInitialization(UWorld* World, const UWorld::InitializationValues IValues)
{
	if (!World)
	{
		return;
	}

	if (!World->IsGameWorld())
	{
		return; 
	}

	UE_LOG(LogAtlantisAudioSubsystem, Log, TEXT("World Init detected for Game World: [%s] (%s)"), *World->GetName(), *World->GetMapName());

	TrackedWorld = World;

	// Hook into this specific world's Begin Play
	WorldBeginPlayHandle = World->OnWorldBeginPlay.AddUObject(this, &UAtlantisAudioSubsystem::HandleWorldBeginPlay);

	// Hook into the global Cleanup delegate to track when this world winds down
	WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &UAtlantisAudioSubsystem::HandleWorldCleanup);
}

void UAtlantisAudioSubsystem::HandleWorldBeginPlay()
{
    if (!TrackedWorld.IsValid())
    {
        UE_LOG(LogAtlantisAudioSubsystem, Warning, TEXT("HandleWorldBeginPlay fired but TrackedWorld is invalid!"));
        return;
    }

    // Bypassed check
    if (!bIsActive)
    {
        UE_LOG(LogAtlantisAudioSubsystem, Log, TEXT("World [%s] began play, but music playback is disabled via settings."), *TrackedWorld->GetName());
        return;
    }

    UE_LOG(LogAtlantisAudioSubsystem, Log, TEXT("World [%s] Begin Play active. Instantiating speakers and reading audio settings..."), *TrackedWorld->GetName());
    
    // Your music spawning logic goes here (use TrackedWorld.Get() if you need a world reference)
	USoundBase* MusicAsset = Cast<USoundBase>(UAtlantisAudioDeveloperSettings::GetMusicMetaSoundSource());
	if (MusicAsset)
	{
		MusicAudioComponent = UGameplayStatics::SpawnSound2D(TrackedWorld.Get(), MusicAsset, 1.f, 1.f, 0.f, nullptr, false, true);
	
		if (MusicAudioComponent)
		{
			GeneratorHandle = UMetasoundGeneratorHandle::CreateMetaSoundGeneratorHandle(MusicAudioComponent);

			TrackedWorld->GetTimerManager().SetTimer(SweepTimerHandle, this, &UAtlantisAudioSubsystem::UpdateAxisTestSweep, 0.033f, true);
		}
	
	}
}

void UAtlantisAudioSubsystem::HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	if (World == TrackedWorld.Get())
	{
		UE_LOG(LogAtlantisAudioSubsystem, Log, TEXT("Cleaning up tracked world [%s]. Flushing components..."), *World->GetName());

		if (MusicAudioComponent)
		{
			MusicAudioComponent->Stop();
			MusicAudioComponent = nullptr;
			UE_LOG(LogAtlantisAudioSubsystem, Log, TEXT("-> Music Speaker safely stopped and reference cleared."));
		}

		// Remove the world-specific delegate
		World->OnWorldBeginPlay.Remove(WorldBeginPlayHandle);
		
		// Remove the global cleanup delegate
		FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);

		TrackedWorld.Reset();
		GeneratorHandle = nullptr;
	}
}

void UAtlantisAudioSubsystem::UpdateAxisTestSweep()
{
	if(!MusicAudioComponent)
	{
		return;
	}

	if(CVarTrackAtlantisAudioAxisSweep.GetValueOnGameThread() != 1)
	{
		return;
	}

	RunningSweepTime += 0.01f;
	float ModTime = FMath::Fmod(RunningSweepTime, 2.0f);
	float TestValue = 1.0f - FMath::Abs(ModTime - 1.0f);

	MusicAudioComponent->SetFloatParameter(TEXT("Atlantis.Music.Axes.PlayerState"), TestValue);
	MusicAudioComponent->SetFloatParameter(TEXT("Atlantis.Music.Axes.Tension"), TestValue);
	MusicAudioComponent->SetFloatParameter(TEXT("Atlantis.Music.Axes.Environment"), TestValue);
	
	UE_LOG(LogAtlantisAudioSubsystem, Verbose, TEXT("Atlantis Audio Subsystem active debugging axis values update: %f"), TestValue);
}