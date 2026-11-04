// Fill out your copyright notice in the Description page of Project Settings.

#include "Audio/AtlantisAudioSubsystem.h"

#include "Audio/AtlantisAudioDeveloperSettings.h"
#include "Audio/MetaSound/Interfaces/AtlantisAudioInterface_MusicAxes.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MetasoundSource.h"
#include "MetasoundGeneratorHandle.h"
#include "Audio/AtlantisAudioFactTags.h"
#include "Audio/PubSubSystem/ASGC_DataRouterSubsystem.h"
#include "Audio/AtlantisAudioPayloadTypes.h"
#include "StructUtils/InstancedStruct.h"
#include "Audio/AtlantisMusicAxisMapping.h"



DEFINE_LOG_CATEGORY(LogAtlantisAudioSubsystem);

static TAutoConsoleVariable<int32> CVarTrackAtlantisAudioAxisSweep(
	TEXT("Atlantis.Audio.AxesSweepTest"),
	0,
	TEXT("Enables a slow 0 to 1 automated parameter sweep for debugging the music subsystem axes.\n")
	TEXT("0: Disabled (Default)\n")
	TEXT("1: Enabled"),
	ECVF_Cheat
);


void UAtlantisAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
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
		PostWorldInitHandle.Reset();
	}

	StopTrackingWorld();

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

	// This delegate is global, so ignore worlds owned by another game instance (for example multi-instance PIE).
	if (World->GetGameInstance() != GetGameInstance())
	{
		UE_LOG(LogAtlantisAudioSubsystem, Log, TEXT("Ignoring world [%s]: it belongs to a different game instance."), *World->GetName());
		return;
	}

	// Release any previous world's subscriptions before replacing the stored handles.
	StopTrackingWorld();

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

	if (!bIsActive)
	{
		UE_LOG(LogAtlantisAudioSubsystem, Log, TEXT("World [%s] began play, but music playback is disabled via settings."), *TrackedWorld->GetName());
		return;
	}

	UE_LOG(LogAtlantisAudioSubsystem, Log, TEXT("World [%s] Begin Play active. Instantiating speakers and reading audio settings..."), *TrackedWorld->GetName());

	AxisMapping = UAtlantisAudioDeveloperSettings::GetMusicAxisMapping();
	if (!AxisMapping)
	{
		UE_LOG(LogAtlantisAudioSubsystem, Error, TEXT("No Music Axis Mapping is set. Assign one in Project Settings > Atlantis Audio Settings. Music won't react to gameplay."));
	}

	SubscribeToFacts();

	USoundBase* MusicAsset = Cast<USoundBase>(UAtlantisAudioDeveloperSettings::GetMusicMetaSoundSource());
	if (!MusicAsset)
	{
		UE_LOG(LogAtlantisAudioSubsystem, Error, TEXT("No music MetaSound Source is set. Assign one in Project Settings > Atlantis Audio Settings."));
		return;
	}

	MusicAudioComponent = UGameplayStatics::SpawnSound2D(TrackedWorld.Get(), MusicAsset, 1.f, 1.f, 0.f, nullptr, false, true);
	if (!MusicAudioComponent)
	{
		UE_LOG(LogAtlantisAudioSubsystem, Error, TEXT("Failed to spawn the music audio component for asset [%s]."), *MusicAsset->GetName());
		return;
	}

	// Created here for the parameter pack pipeline (#105) to use later. The sweep below goes through the component.
	GeneratorHandle = UMetasoundGeneratorHandle::CreateMetaSoundGeneratorHandle(MusicAudioComponent);

	UpdateMusicAxes();

	TrackedWorld->GetTimerManager().SetTimer(SweepTimerHandle, this, &UAtlantisAudioSubsystem::UpdateAxisTestSweep, SweepTimerInterval, true);
}

void UAtlantisAudioSubsystem::HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{

	if (World != TrackedWorld.Get())
	{
		return;
	}

	StopTrackingWorld();
}

void UAtlantisAudioSubsystem::UpdateAxisTestSweep()
{
	if (!MusicAudioComponent)
	{
		return;
	}

	if (CVarTrackAtlantisAudioAxisSweep.GetValueOnGameThread() != 1)
	{
		return;
	}

	RunningSweepTime += SweepTimerInterval;
	const float ModTime = FMath::Fmod(RunningSweepTime, 2.0f);
	const float TestValue = 1.0f - FMath::Abs(ModTime - 1.0f);

	// Pin names come from the interface declaration, so a typo cannot silently break the sweep.
	using namespace ProjectAtlantisAudio::MusicAxesInterface;
	MusicAudioComponent->SetFloatParameter(Inputs::PlayerStateAxis.Resolve(), TestValue);
	MusicAudioComponent->SetFloatParameter(Inputs::TensionAxis.Resolve(), TestValue);
	MusicAudioComponent->SetFloatParameter(Inputs::EnvironmentAxis.Resolve(), TestValue);

	UE_LOG(LogAtlantisAudioSubsystem, Verbose, TEXT("Atlantis Audio Subsystem active debugging axis values update: %f"), TestValue);
}

void UAtlantisAudioSubsystem::StopTrackingWorld()
{
	UnsubscribeFromFacts();

	if (TrackedWorld.Get())
	{
		UE_LOG(LogAtlantisAudioSubsystem, Log, TEXT("Cleaning up tracked world [%s]. Flushing components..."), *TrackedWorld->GetName());

		TrackedWorld->OnWorldBeginPlay.Remove(WorldBeginPlayHandle);
		TrackedWorld->GetTimerManager().ClearTimer(SweepTimerHandle);
	}

	WorldBeginPlayHandle.Reset();

	if (WorldCleanupHandle.IsValid())
	{
		FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
		WorldCleanupHandle.Reset();
	}

	if (MusicAudioComponent)
	{
		MusicAudioComponent->Stop();
		MusicAudioComponent = nullptr;
		UE_LOG(LogAtlantisAudioSubsystem, Log, TEXT("-> Music Speaker safely stopped and reference cleared."));
	}

	TrackedWorld.Reset();
	GeneratorHandle = nullptr;
	RunningSweepTime = 0.f;
	FloatFacts.Reset();
	BoolFacts.Reset();
	KeyedStateFacts.Reset();
	AxisMapping = nullptr;
}

void UAtlantisAudioSubsystem::OnGameplayFactReceived_Implementation(FGameplayTag Tag, const FInstancedStruct& Payload)
{
	UE_LOG(LogAtlantisAudioSubsystem, Verbose, TEXT("Fact received: [%s]"), *Tag.ToString());

	if (const FAtlantisAudioFloatPayload* FloatData = Payload.GetPtr<FAtlantisAudioFloatPayload>())
	{
		FloatFacts.Add(Tag,FloatData->Value);
	} 
	else if (const FAtlantisAudioBoolPayload* BoolData = Payload.GetPtr<FAtlantisAudioBoolPayload>())
	{
		BoolFacts.Add(Tag, BoolData->bValue);
	} 
	else if (const FAtlantisAudioKeyedStatePayload* KeyedData = Payload.GetPtr<FAtlantisAudioKeyedStatePayload>())
	{
		KeyedStateFacts.FindOrAdd(Tag).Add(KeyedData->Key, KeyedData->State);
	} 
	else
	{
		UE_LOG(LogAtlantisAudioSubsystem, Warning, TEXT("Fact [%s] holds a payload shape the music subsystem doesn't handle."), *Tag.ToString());
	}
	UpdateMusicAxes();
}

void UAtlantisAudioSubsystem::SubscribeToFacts()
{
	if(!TrackedWorld.Get())
	{
		UE_LOG(LogAtlantisAudioSubsystem, Error, TEXT("TrackedWorld unavailable when subscribing to facts!"));
        return;
	}

	UASGC_DataRouterSubsystem* Router = TrackedWorld->GetSubsystem<UASGC_DataRouterSubsystem>();
    if (!Router)
    {
        UE_LOG(LogAtlantisAudioSubsystem, Error, TEXT("No Data Router in world [%s]. Music won't react to gameplay facts."), *TrackedWorld->GetName());
        return;
    }

	const FGameplayTag FactTags[] = {
		TAG_AudioFact_Oxygen_Level,
		TAG_AudioFact_Oxygen_Critical,
		TAG_AudioFact_Stealth_Hiding,
		TAG_AudioFact_Investigation_Status,
		TAG_AudioFact_Enemy_Status};

	for(const FGameplayTag& Tag : FactTags)
	{
		int32 Handle = Router->Subscribe(Tag, this);
		FactSubscriptions.Add(Tag, Handle);
	}

}

void UAtlantisAudioSubsystem::UnsubscribeFromFacts()
{
	if (!TrackedWorld.Get())
	{
		FactSubscriptions.Reset();
		return;
	}


	UASGC_DataRouterSubsystem* Router = TrackedWorld->GetSubsystem<UASGC_DataRouterSubsystem>();
    if (!Router)
    {
		FactSubscriptions.Reset();
		return;
    }

	for(const TPair<FGameplayTag, int32>& Subscription : FactSubscriptions)
	{
		Router->Unsubscribe(Subscription.Key, Subscription.Value);
	}

	FactSubscriptions.Reset();

}


float UAtlantisAudioSubsystem::GetRuleContribution(const FAtlantisMusicAxisRule& Rule) const
{
	if (const float* Value = FloatFacts.Find(Rule.Tag))
	{
		return Rule.Amount * (Rule.bInvertValue ? 1.f - *Value : *Value);
	}

	if(const bool* bValue = BoolFacts.Find(Rule.Tag))
	{
		return *bValue ? Rule.Amount : 0.f;
	}

	// A keyed rule's tag is a state, so check whether anything is in that state right now.
	for (const TPair<FGameplayTag, TMap<FName, FGameplayTag>>& Fact : KeyedStateFacts)
	{
		if (Fact.Value.FindKey(Rule.Tag))
		{
			return Rule.Amount;
		}
	}

	return 0.f;
}

void UAtlantisAudioSubsystem::UpdateMusicAxes()
{
	if (!AxisMapping)
	{
		return;
	}

	float PlayerState = AxisMapping->PlayerStateBaseline;
	float Tension = AxisMapping->TensionBaseline;

	for (const FAtlantisMusicAxisRule& Rule : AxisMapping->Rules)
	{
		const float Contribution = GetRuleContribution(Rule);

		if (Rule.Axis == EAtlantisMusicAxis::PlayerState)
		{
			PlayerState = FMath::Max(PlayerState, Contribution);
		}
		else
		{
			Tension += Contribution;
		}
	}

	PlayerState = FMath::Clamp(PlayerState, 0.f, 1.f);
	Tension = FMath::Clamp(Tension, 0.f, 1.f);
	CurrentPlayerState = PlayerState;
	CurrentTension = Tension;

	UE_LOG(LogAtlantisAudioSubsystem, Verbose, TEXT("Music axes: PlayerState %.2f, Tension %.2f"), PlayerState, Tension);

	if (!MusicAudioComponent)
	{
		return;
	}

	using namespace ProjectAtlantisAudio::MusicAxesInterface;
	MusicAudioComponent->SetFloatParameter(Inputs::PlayerStateAxis.Resolve(), PlayerState);
	MusicAudioComponent->SetFloatParameter(Inputs::TensionAxis.Resolve(), Tension);
}

