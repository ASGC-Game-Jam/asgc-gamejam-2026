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

static constexpr float LoopFadeOutSeconds = 0.3f;

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
		TAG_AudioFact_Player_Present,
		TAG_AudioFact_Player_Underwater};

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
	StopPlayerLoops();
	BoolFacts.Reset();
	FloatFacts.Reset();

	Super::Deinitialize();
}

void UAtlantisPlayerAudioSubsystem::OnGameplayFactReceived_Implementation(FGameplayTag Tag, const FInstancedStruct& Payload)
{
	// A bool fact can start or stop a loop, so recheck them all.
	const FAtlantisAudioBoolPayload* BoolFactPtr = Payload.GetPtr<FAtlantisAudioBoolPayload>();
	if (BoolFactPtr)
	{
		BoolFacts.Add(Tag, BoolFactPtr->bValue);
		RefreshPlayerLoops();
	}

	const FAtlantisAudioFloatPayload* FloatFactPtr = Payload.GetPtr<FAtlantisAudioFloatPayload>();
	if (FloatFactPtr)
	{
		// Store the value so a loop that starts later still gets it.
		FloatFacts.Add(Tag, FloatFactPtr->Value);

		if (!PlayerAudio)
		{
			return;
		}

		TArray<FAtlantisPlayerAudioRow*> PlayerAudioRows;
		PlayerAudio->GetAllRows<FAtlantisPlayerAudioRow>(TEXT("OnGameplayFactReceived"), PlayerAudioRows);

		if (PlayerAudioRows.IsEmpty())
		{
			return;
		}

		for (const FAtlantisPlayerAudioRow* Row : PlayerAudioRows)
		{
			if (!Row->Sound || Row->ParameterFact != Tag)
			{
				continue;
			}

			// Null if the loop isn't playing. ApplyParameter skips it then.
			UAudioComponent* AudioComp = PlayingPlayerAudio.FindRef(Row->Sound);
			ApplyParameter(*Row, AudioComp);
		}
	}
}

bool UAtlantisPlayerAudioSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UAtlantisPlayerAudioSubsystem::RefreshPlayerLoops()
{
	if (!PlayerAudio || !GetWorld())
	{
		return;
	}

	TArray<FAtlantisPlayerAudioRow*> PlayerAudioRows;
	PlayerAudio->GetAllRows<FAtlantisPlayerAudioRow>(TEXT("RefreshPlayerLoops"), PlayerAudioRows);

	if (PlayerAudioRows.IsEmpty())
	{
		return;
	}

	for (const FAtlantisPlayerAudioRow* Row : PlayerAudioRows)
	{
		if (!Row->Sound || Row->Behaviour != EAtlantisPlayerAudioBehaviour::Loop)
		{
			continue;
		}

		const bool bShouldPlay = ShouldLoopPlay(*Row);
		const bool bIsPlaying = PlayingPlayerAudio.Contains(Row->Sound);

		if (bShouldPlay && !bIsPlaying)
		{
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
				continue;
			}

			// Keeps playing while the journal or pause menu pauses the game.
			AudioComp->bIsUISound = true;
			AudioComp->Play();

			// Send the current value straight away, so a re-dive doesn't start from the MetaSound's default.
			ApplyParameter(*Row, AudioComp);
			PlayingPlayerAudio.Add(Row->Sound, AudioComp);
		}
		else if (!bShouldPlay && bIsPlaying)
		{
			UAudioComponent* AudioComp = PlayingPlayerAudio.FindRef(Row->Sound);
			// The component destroys itself once the fade ends.
			if (IsValid(AudioComp))
			{
				AudioComp->FadeOut(LoopFadeOutSeconds, 0.f);
			}
			PlayingPlayerAudio.Remove(Row->Sound);
		}
	}
}

void UAtlantisPlayerAudioSubsystem::StopPlayerLoops()
{
	for (const auto& Pair : PlayingPlayerAudio)
	{
		if (IsValid(Pair.Value))
		{
			Pair.Value->Stop();
		}
	}
	PlayingPlayerAudio.Reset();
}

bool UAtlantisPlayerAudioSubsystem::ShouldLoopPlay(const FAtlantisPlayerAudioRow& Row) const
{
	// FindRef gives false for a fact that hasn't arrived yet.
	return BoolFacts.FindRef(TAG_AudioFact_Player_Present)
		&& BoolFacts.FindRef(Row.PlayWhileFact)
		&& (!Row.bUnderwaterOnly || BoolFacts.FindRef(TAG_AudioFact_Player_Underwater));
}

void UAtlantisPlayerAudioSubsystem::ApplyParameter(const FAtlantisPlayerAudioRow& Row, UAudioComponent* AudioComp) const
{
	if (Row.ParameterName == NAME_None)
	{
		return;
	}

	// Null if the fact hasn't arrived yet. The MetaSound keeps its default until it does.
	const float* FactPtr = FloatFacts.Find(Row.ParameterFact);

	if (!FactPtr || !IsValid(AudioComp))
	{
		return;
	}

	AudioComp->SetFloatParameter(Row.ParameterName, *FactPtr);
}