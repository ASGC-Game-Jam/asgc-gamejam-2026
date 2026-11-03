// Copyright (c) 2026 ASGC

#if WITH_DEV_AUTOMATION_TESTS

#include "Audio/AtlantisAudioDataBPFL.h"
#include "Audio/PubSubSystem/ASGC_DataRouterSubsystem.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "NativeGameplayTags.h"
#include "StructUtils/InstancedStruct.h"
#include "Tests/AtlantisAudioTestAccess.h"
#include "Tests/AtlantisAudioTestFactListener.h"
#include "UObject/StrongObjectPtr.h"
#include "Audio/AtlantisAudioPayloadTypes.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"


namespace
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Test_AudioIngestion_Vector, "Test.Atlantis.AudioIngestion.Vector");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Test_AudioIngestion_NoSubscribers, "Test.Atlantis.AudioIngestion.NoSubscribers");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Test_AudioIngestion_Unregistered, "Test.Atlantis.AudioIngestion.Unregistered");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Test_AudioIngestion_Keyed, "Test.Atlantis.AudioIngestion.Keyed");

}

BEGIN_DEFINE_SPEC(FAtlantisAudioDataIngestionSpec, "Atlantis.Audio.DataIngestion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	FAtlantisAudioPayloadRegistry SavedRegistry;
	TStrongObjectPtr<UWorld> World;
	TStrongObjectPtr<UAtlantisAudioTestFactListener> Listener;
	int32 SubscriptionHandle = INDEX_NONE;
	int32 KeyedSubscriptionHandle = INDEX_NONE;
END_DEFINE_SPEC(FAtlantisAudioDataIngestionSpec)

void FAtlantisAudioDataIngestionSpec::Define()
{
	BeforeEach([this]()
	{
		// Swap in test rows, same as the validation spec.
		FAtlantisAudioPayloadRegistry& Registry = FAtlantisAudioValidationTestAccess::Registry();
		SavedRegistry = Registry;
		Registry.Reset();
		Registry.Add(TAG_Test_AudioIngestion_Vector, TBaseStructure<FVector>::Get());
		Registry.Add(TAG_Test_AudioIngestion_NoSubscribers, TBaseStructure<FVector>::Get());
		Registry.Add(TAG_Test_AudioIngestion_Keyed, FAtlantisAudioKeyedStatePayload::StaticStruct());

		// A throwaway world gets its own router, so tests never touch the editor's.
		World.Reset(UWorld::CreateWorld(EWorldType::Game, false));
		Listener.Reset(NewObject<UAtlantisAudioTestFactListener>());

		if (TestNotNull(TEXT("Test world created"), World.Get()))
		{
			UASGC_DataRouterSubsystem* Router = World->GetSubsystem<UASGC_DataRouterSubsystem>();
			if (TestNotNull(TEXT("Test world has a router"), Router))
			{
				SubscriptionHandle = Router->Subscribe(TAG_Test_AudioIngestion_Vector, Listener.Get());
				KeyedSubscriptionHandle = Router->Subscribe(TAG_Test_AudioIngestion_Keyed, Listener.Get());
			}
		}
	});

	AfterEach([this]()
	{
		if (World.IsValid())
		{
			// Unsubscribe first: the router treats a subscriber that vanishes without unsubscribing as a bug.
			if (UASGC_DataRouterSubsystem* Router = World->GetSubsystem<UASGC_DataRouterSubsystem>())
			{
				Router->Unsubscribe(TAG_Test_AudioIngestion_Vector, SubscriptionHandle);
				Router->Unsubscribe(TAG_Test_AudioIngestion_Keyed, KeyedSubscriptionHandle);
			}
			World->DestroyWorld(false);
		}
		World.Reset();
		Listener.Reset();
		SubscriptionHandle = INDEX_NONE;
		KeyedSubscriptionHandle = INDEX_NONE;

		FAtlantisAudioValidationTestAccess::Registry() = SavedRegistry;
	});

	It(TEXT("should route a valid fact to its subscribers"), [this]()
	{
		const FVector Sent(1.f, 2.f, 3.f);
		const bool bAccepted = UAtlantisAudioDataBPFL::SetAudioDataPayload(World.Get(), TAG_Test_AudioIngestion_Vector, FInstancedStruct::Make(Sent));

		TestTrue(TEXT("Valid fact is accepted"), bAccepted);
		TestEqual(TEXT("Subscriber received exactly one fact"), Listener->ReceivedCount, 1);
		TestTrue(TEXT("Subscriber received the right tag"), Listener->LastTag == TAG_Test_AudioIngestion_Vector);

		const FVector* Received = Listener->LastPayload.GetPtr<FVector>();
		if (TestNotNull(TEXT("Payload still holds a vector"), Received))
		{
			TestEqual(TEXT("Payload value survives the trip"), *Received, Sent);
		}
	});

	Describe(TEXT("Rejected facts"), [this]()
	{
        It(TEXT("should reject and not route an unregistered tag"), [this]()
        {
            AddExpectedErrorPlain(TEXT("is not registered"), EAutomationExpectedErrorFlags::Contains, 0);

            const bool bAccepted = UAtlantisAudioDataBPFL::SetAudioDataPayload(World.Get(), TAG_Test_AudioIngestion_Unregistered, FInstancedStruct::Make(FVector::ZeroVector));

            TestFalse(TEXT("Unregistered tag is rejected"), bAccepted);
            TestEqual(TEXT("Subscriber received nothing"), Listener->ReceivedCount, 0);
        });

        It(TEXT("should reject and not route a payload holding the wrong struct"), [this]()
        {
            AddExpectedErrorPlain(TEXT("but the registry expects"), EAutomationExpectedErrorFlags::Contains, 0);

            const bool bAccepted = UAtlantisAudioDataBPFL::SetAudioDataPayload(World.Get(), TAG_Test_AudioIngestion_Vector, FInstancedStruct::Make(FRotator::ZeroRotator));

            TestFalse(TEXT("Mismatched payload is rejected"), bAccepted);
            TestEqual(TEXT("Subscriber received nothing"), Listener->ReceivedCount, 0);
        });

        It(TEXT("should reject a null world context"), [this]()
        {
            AddExpectedErrorPlain(TEXT("has no valid world"), EAutomationExpectedErrorFlags::Contains, 0);
            AddExpectedErrorPlain(TEXT("null object was passed as a world context"), EAutomationExpectedErrorFlags::Contains, 1);

            const bool bAccepted = UAtlantisAudioDataBPFL::SetAudioDataPayload(nullptr, TAG_Test_AudioIngestion_Vector, FInstancedStruct::Make(FVector::ZeroVector));

            TestFalse(TEXT("Null World is rejected"), bAccepted);
            TestEqual(TEXT("Subscriber received nothing"), Listener->ReceivedCount, 0);
        });

	});

    It(TEXT("should accept a registered tag that has no subscribers"), [this]()
    {
        const bool bAccepted = UAtlantisAudioDataBPFL::SetAudioDataPayload(World.Get(), TAG_Test_AudioIngestion_NoSubscribers, FInstancedStruct::Make(FVector::ZeroVector));

        TestTrue(TEXT("Fact with no subscribers is accepted"), bAccepted);
        TestEqual(TEXT("Subscriber received nothing"), Listener->ReceivedCount, 0);
    });

	Describe(TEXT("Keyed payload Key"), [this]()
	{
		It(TEXT("should use the calling actor's name when Key is empty"), [this]()
		{
			const AActor* Caller = World->SpawnActor<AActor>();

			UAtlantisAudioDataBPFL::SetAudioDataPayload(Caller, TAG_Test_AudioIngestion_Keyed, FInstancedStruct::Make(FAtlantisAudioKeyedStatePayload()));

			const FAtlantisAudioKeyedStatePayload* Received = Listener->LastPayload.GetPtr<FAtlantisAudioKeyedStatePayload>();
			if (TestNotNull(TEXT("Payload still holds a keyed state"), Received))
			{
				TestTrue(TEXT("Key is the caller's name"), Received->Key == Caller->GetFName());
			}
		});

		It(TEXT("should use the owning actor's name when a component calls"), [this]()
		{
			AActor* Caller = World->SpawnActor<AActor>();
			const USceneComponent* Component = NewObject<USceneComponent>(Caller);

			UAtlantisAudioDataBPFL::SetAudioDataPayload(Component, TAG_Test_AudioIngestion_Keyed, FInstancedStruct::Make(FAtlantisAudioKeyedStatePayload()));

			const FAtlantisAudioKeyedStatePayload* Received = Listener->LastPayload.GetPtr<FAtlantisAudioKeyedStatePayload>();
			if (TestNotNull(TEXT("Payload still holds a keyed state"), Received))
			{
				TestTrue(TEXT("Key is the owning actor's name"), Received->Key == Caller->GetFName());
			}
		});

		It(TEXT("should keep a Key that was set"), [this]()
		{
			const AActor* Caller = World->SpawnActor<AActor>();
			FAtlantisAudioKeyedStatePayload Data;
			Data.Key = TEXT("EnemyA");

			UAtlantisAudioDataBPFL::SetAudioDataPayload(Caller, TAG_Test_AudioIngestion_Keyed, FInstancedStruct::Make(Data));

			const FAtlantisAudioKeyedStatePayload* Received = Listener->LastPayload.GetPtr<FAtlantisAudioKeyedStatePayload>();
			if (TestNotNull(TEXT("Payload still holds a keyed state"), Received))
			{
				TestTrue(TEXT("The set Key wins"), Received->Key == FName(TEXT("EnemyA")));
			}
		});
	});

}

#endif
