// Copyright (c) 2026 ASGC

#if WITH_DEV_AUTOMATION_TESTS

#include "Audio/AtlantisAudioPayloadTypes.h"
#include "Audio/AtlantisAudioSubsystem.h"
#include "Audio/AtlantisMusicAxisMapping.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"
#include "NativeGameplayTags.h"
#include "StructUtils/InstancedStruct.h"
#include "Tests/AtlantisAudioTestAccess.h"
#include "UObject/StrongObjectPtr.h"
#include "Audio/AtlantisAudioFactTags.h"
#include "Audio/PubSubSystem/ASGC_DataRouterSubsystem.h"
#include "Engine/World.h"


namespace
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Test_MusicAxes_Float, "Test.Atlantis.MusicAxes.Float");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Test_MusicAxes_Bool, "Test.Atlantis.MusicAxes.Bool");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Test_MusicAxes_Keyed, "Test.Atlantis.MusicAxes.Keyed");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Test_MusicAxes_StateLow, "Test.Atlantis.MusicAxes.StateLow");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Test_MusicAxes_StateHigh, "Test.Atlantis.MusicAxes.StateHigh");
}

BEGIN_DEFINE_SPEC(FAtlantisAudioMusicAxesSpec, "Atlantis.Audio.MusicAxes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	TStrongObjectPtr<UGameInstance> GameInstance;
	TStrongObjectPtr<UAtlantisAudioSubsystem> Subsystem;
	TStrongObjectPtr<UAtlantisMusicAxisMapping> Mapping;

	void AddRule(FGameplayTag Tag, EAtlantisMusicAxis Axis, float Amount, bool bInvert = false)
	{
		FAtlantisMusicAxisRule Rule;
		Rule.Tag = Tag;
		Rule.Axis = Axis;
		Rule.Amount = Amount;
		Rule.bInvertValue = bInvert;
		Mapping->Rules.Add(Rule);
	}

	void SendFloat(FGameplayTag Tag, float Value)
	{
		FAtlantisAudioFloatPayload Data;
		Data.Value = Value;
		Subsystem->OnGameplayFactReceived_Implementation(Tag, FInstancedStruct::Make(Data));
	}

	void SendBool(FGameplayTag Tag, bool bValue)
	{
		FAtlantisAudioBoolPayload Data;
		Data.bValue = bValue;
		Subsystem->OnGameplayFactReceived_Implementation(Tag, FInstancedStruct::Make(Data));
	}

	void SendKeyed(FName Key, FGameplayTag State)
	{
		FAtlantisAudioKeyedStatePayload Data;
		Data.Key = Key;
		Data.State = State;
		Subsystem->OnGameplayFactReceived_Implementation(TAG_Test_MusicAxes_Keyed, FInstancedStruct::Make(Data));
	}

	void ExpectAxes(float ExpectedPlayerState, float ExpectedTension)
	{
		TestEqual(TEXT("PlayerState"), FAtlantisAudioSubsystemTestAccess::PlayerState(Subsystem.Get()), ExpectedPlayerState);
		TestEqual(TEXT("Tension"), FAtlantisAudioSubsystemTestAccess::Tension(Subsystem.Get()), ExpectedTension);
	}
END_DEFINE_SPEC(FAtlantisAudioMusicAxesSpec)

void FAtlantisAudioMusicAxesSpec::Define()
{
	BeforeEach([this]()
	{
		// A subsystem has to live inside a game instance, so give it a throwaway one.
		GameInstance.Reset(NewObject<UGameInstance>(GEngine));
		Subsystem.Reset(NewObject<UAtlantisAudioSubsystem>(GameInstance.Get()));
		Mapping.Reset(NewObject<UAtlantisMusicAxisMapping>());

		// A fixed sheet, so changing the real data asset can't break these tests.
		Mapping->PlayerStateBaseline = 0.5f;
		Mapping->TensionBaseline = 0.3f;
		AddRule(TAG_Test_MusicAxes_Float, EAtlantisMusicAxis::Tension, -0.3f, true);
		AddRule(TAG_Test_MusicAxes_Bool, EAtlantisMusicAxis::Tension, 0.3f);
		AddRule(TAG_Test_MusicAxes_StateLow, EAtlantisMusicAxis::PlayerState, 0.7f);
		AddRule(TAG_Test_MusicAxes_StateLow, EAtlantisMusicAxis::Tension, 0.2f);
		AddRule(TAG_Test_MusicAxes_StateHigh, EAtlantisMusicAxis::PlayerState, 1.f);

		FAtlantisAudioSubsystemTestAccess::SetMapping(Subsystem.Get(), Mapping.Get());
	});

	AfterEach([this]()
	{
		Subsystem.Reset();
		Mapping.Reset();
		GameInstance.Reset();
	});

	It(TEXT("should rest at the baselines before any fact arrives"), [this]()
	{
		FAtlantisAudioSubsystemTestAccess::UpdateAxes(Subsystem.Get());

		ExpectAxes(0.5f, 0.3f);
	});

	It(TEXT("should use the highest PlayerState across all instances"), [this]()
	{
		SendKeyed(TEXT("EnemyA"), TAG_Test_MusicAxes_StateLow);
		SendKeyed(TEXT("EnemyB"), TAG_Test_MusicAxes_StateHigh);

		ExpectAxes(1.f, 0.5f);
	});

	It(TEXT("should invert a float rule"), [this]()
	{
		SendFloat(TAG_Test_MusicAxes_Float, 0.2f);

		ExpectAxes(0.5f, 0.06f);
	});

    It(TEXT("should only apply a bool rule while true"), [this]()
	{
		SendBool(TAG_Test_MusicAxes_Bool, true);
        ExpectAxes(0.5f, 0.6f);

        SendBool(TAG_Test_MusicAxes_Bool, false);
		ExpectAxes(0.5f, 0.3f);
	});

    It(TEXT("should replace an instance's previous state"), [this]()
	{
		SendKeyed(TEXT("EnemyA"), TAG_Test_MusicAxes_StateHigh);
		SendKeyed(TEXT("EnemyA"), TAG_Test_MusicAxes_StateLow);

		ExpectAxes(0.7f, 0.5f);
	});

    It(TEXT("should count a shared state only once"), [this]()
	{
		SendKeyed(TEXT("EnemyA"), TAG_Test_MusicAxes_StateLow);
		SendKeyed(TEXT("EnemyB"), TAG_Test_MusicAxes_StateLow);

		ExpectAxes(0.7f, 0.5f);
	});

    It(TEXT("should clamp tension to 1"), [this]()
	{
        AddRule(TAG_Test_MusicAxes_Bool, EAtlantisMusicAxis::Tension, 5.f);
        SendBool(TAG_Test_MusicAxes_Bool, true);

		ExpectAxes(0.5f, 1.0f);
	});

    It(TEXT("should forget facts when tracking stops"), [this]()
	{
		SendBool(TAG_Test_MusicAxes_Bool, true);
		FAtlantisAudioSubsystemTestAccess::StopTracking(Subsystem.Get());
		FAtlantisAudioSubsystemTestAccess::SetMapping(Subsystem.Get(), Mapping.Get());
		FAtlantisAudioSubsystemTestAccess::UpdateAxes(Subsystem.Get());

		ExpectAxes(0.5f, 0.3f);
	});

	It(TEXT("should hear facts sent before the world begins play"), [this]()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		World->SetGameInstance(GameInstance.Get());

		FAtlantisAudioSubsystemTestAccess::StartTracking(Subsystem.Get(), World);

		// StartTracking loads the real data asset, so swap the test sheet back in.
		FAtlantisAudioSubsystemTestAccess::SetMapping(Subsystem.Get(), Mapping.Get());
		AddRule(TAG_AudioFact_Stealth_Hiding, EAtlantisMusicAxis::Tension, 0.3f);

		// World BeginPlay never fired, so this is the timing of a fact sent from an actor's BeginPlay.
		FAtlantisAudioBoolPayload Data;
		Data.bValue = true;
		World->GetSubsystem<UASGC_DataRouterSubsystem>()->BroadcastFact(TAG_AudioFact_Stealth_Hiding, FInstancedStruct::Make(Data));

		ExpectAxes(0.5f, 0.6f);

		FAtlantisAudioSubsystemTestAccess::StopTracking(Subsystem.Get());
		World->DestroyWorld(false);
	});

#if WITH_EDITOR
	It(TEXT("should recalculate the axes when the mapping is edited"), [this]()
	{
		FAtlantisAudioSubsystemTestAccess::UpdateAxes(Subsystem.Get());
		Mapping->PlayerStateBaseline = 0.8f;

		FAtlantisAudioSubsystemTestAccess::NotifyPropertyChanged(Subsystem.Get(), Mapping.Get());

		ExpectAxes(0.8f, 0.3f);
	});

		It(TEXT("should ignore edits to other objects"), [this]()
	{
		FAtlantisAudioSubsystemTestAccess::UpdateAxes(Subsystem.Get());
		Mapping->PlayerStateBaseline = 0.8f;

		FAtlantisAudioSubsystemTestAccess::NotifyPropertyChanged(Subsystem.Get(), GameInstance.Get());

		ExpectAxes(0.5f, 0.3f);
	});
#endif

}

#endif
