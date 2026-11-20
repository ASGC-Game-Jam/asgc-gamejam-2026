// Copyright (c) 2026 ASGC

#include "Audio/AtlantisPlayerAudioSubsystem.h"

#include "Audio/AtlantisAudioFactTags.h"
#include "Audio/PubSubSystem/ASGC_DataRouterSubsystem.h"
#include "Audio/AtlantisAudioDeveloperSettings.h"
#include "Audio/AtlantisPlayerAudioTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "Engine/DataTable.h"
#include "Audio/AtlantisAudioPayloadTypes.h"

DEFINE_LOG_CATEGORY_STATIC(LogAtlantisPlayerAudio, Log, All);

void UAtlantisPlayerAudioSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UASGC_DataRouterSubsystem* Router = InWorld.GetSubsystem<UASGC_DataRouterSubsystem>();

	if (!Router)
	{
		UE_LOG(LogAtlantisPlayerAudio, Error, TEXT("No Data Router in world [%s]. Suit sounds won't react to the player."), *InWorld.GetName());
		return;
	}

	const FGameplayTag FactTags[] = {
		TAG_AudioFact_Oxygen_Level,
		TAG_AudioFact_Oxygen_Critical,
		TAG_AudioFact_Player_Present};

	for (const FGameplayTag& Tag : FactTags)
	{
		const int32 Handle = Router->Subscribe(Tag, this);
		if (Handle != INDEX_NONE)
		{
			FactSubscriptions.Add(Tag, Handle);
		}
	}

	PlayerAudio = UAtlantisAudioDeveloperSettings::GetPlayerAudioTable();
	if (!PlayerAudio)
	{
		UE_LOG(LogAtlantisPlayerAudio, Warning, TEXT("No player audio table set in Project Settings."));
		return;
	}
}

void UAtlantisPlayerAudioSubsystem::PreDeinitialize()
{
	if (UWorld* World = GetWorld())
	{
		if (UASGC_DataRouterSubsystem* Router = World->GetSubsystem<UASGC_DataRouterSubsystem>())
		{
			for (const auto& Pair : FactSubscriptions)
			{
				Router->Unsubscribe(Pair.Key, Pair.Value);
			}
		}
	}

	FactSubscriptions.Reset();

	Super::PreDeinitialize();
}

void UAtlantisPlayerAudioSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UAtlantisPlayerAudioSubsystem::OnGameplayFactReceived_Implementation(FGameplayTag Tag, const FInstancedStruct& Payload)
{
	UE_LOG(LogAtlantisPlayerAudio, Log, TEXT("Received Tag [%s]."), *Tag.ToString());
	if (Tag == TAG_AudioFact_Player_Present)
	{
		const FAtlantisAudioBoolPayload* Ptr = Payload.GetPtr<FAtlantisAudioBoolPayload>();
		if (Ptr && Ptr->bValue)
		{
			StartPlayerLoops();
		}
	}
}

bool UAtlantisPlayerAudioSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UAtlantisPlayerAudioSubsystem::StartPlayerLoops()
{

	TArray<FAtlantisPlayerAudioRow*> PlayerAudioRows;
	PlayerAudio->GetAllRows<FAtlantisPlayerAudioRow>(TEXT("StartPlayerLoops"), PlayerAudioRows);

	if(PlayerAudioRows.IsEmpty() || !GetWorld())
	{
		return;
	}

	for (const FAtlantisPlayerAudioRow* Row : PlayerAudioRows)
	{
		if (!Row->Sound || Row->Behaviour != EAtlantisPlayerAudioBehaviour::Loop || PlayingPlayerAudio.Contains(Row->Sound))
		{
			continue;
		}

		UAudioComponent* AudioComp = UGameplayStatics::CreateSound2D(
			this,
			Row->Sound,
			1.0f,
			1.0f,
			0.0f,
			nullptr,
			false,
			true
		);
	
		if (!AudioComp)
		{
			return;
		}

		AudioComp->bIsUISound = true;
		AudioComp->Play();
		PlayingPlayerAudio.Add(Row->Sound, AudioComp);
	}
}

void UAtlantisPlayerAudioSubsystem::StopPlayerLoops()
{
	
}