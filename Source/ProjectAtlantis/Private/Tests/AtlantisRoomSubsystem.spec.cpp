// Copyright (c) 2026 ASGC

#if WITH_DEV_AUTOMATION_TESTS

#include "Audio/AtlantisAudioFactTags.h"
#include "Audio/AtlantisAudioPayloadTypes.h"
#include "Audio/AtlantisRoomSubsystem.h"
#include "Audio/PubSubSystem/ASGC_DataRouterSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "StructUtils/InstancedStruct.h"
#include "Tests/AtlantisAudioTestFactListener.h"
#include "UObject/StrongObjectPtr.h"
#include "GameFramework/Actor.h"

namespace
{
	// A throwaway world with its own router and room subsystem, plus a listener that records room facts.
	struct FAtlantisRoomFixture
	{
		TStrongObjectPtr<UGameInstance> GameInstance;
		UWorld* World = nullptr;
		UASGC_DataRouterSubsystem* Router = nullptr;
		UAtlantisRoomSubsystem* Rooms = nullptr;
		TStrongObjectPtr<UAtlantisAudioTestFactListener> Listener;
		int32 SubscriptionHandle = INDEX_NONE;

		// Stand-ins for two volumes. The subsystem only uses them to tell volumes apart.
		// The world owns them, so they live until it's destroyed.
		AActor* VolumeA = nullptr;
		AActor* VolumeB = nullptr;

		FAtlantisRoomFixture()
			: GameInstance(NewObject<UGameInstance>(GEngine))
			, Listener(NewObject<UAtlantisAudioTestFactListener>())
		{
			GameInstance->InitializeStandalone();
			World = GameInstance->GetWorld();
			if (!World)
			{
				return;
			}

			VolumeA = World->SpawnActor<AActor>();
			VolumeB = World->SpawnActor<AActor>();

			Router = World->GetSubsystem<UASGC_DataRouterSubsystem>();
			Rooms = World->GetSubsystem<UAtlantisRoomSubsystem>();
			if (Router)
			{
				SubscriptionHandle = Router->Subscribe(TAG_AudioFact_Environment_Room, Listener.Get());
			}
		}

		~FAtlantisRoomFixture()
		{
			// Unsubscribe first: the router treats a subscriber that vanishes without unsubscribing as a bug.
			if (Router)
			{
				Router->Unsubscribe(TAG_AudioFact_Environment_Room, SubscriptionHandle);
			}

			GameInstance->Shutdown();
			if (World)
			{
				GEngine->DestroyWorldContext(World);
				World->DestroyWorld(false);
			}
		}

		/** How many room facts have been sent. */
		int32 Count() const
		{
			return Listener->ReceivedCount;
		}

		/** The room in the most recent room fact, or an empty tag if none was sent. */
		FGameplayTag LastRoom() const
		{
			const FAtlantisAudioTagPayload* Payload = Listener->LastPayload.GetPtr<FAtlantisAudioTagPayload>();
			return Payload ? Payload->Tag : FGameplayTag();
		}
	};
}

BEGIN_DEFINE_SPEC(FAtlantisRoomSubsystemSpec, "Atlantis.Audio.Rooms",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	TUniquePtr<FAtlantisRoomFixture> Fixture;
	FGameplayTag Open;
	FGameplayTag Small;
END_DEFINE_SPEC(FAtlantisRoomSubsystemSpec)

void FAtlantisRoomSubsystemSpec::Define()
{
	BeforeEach([this]()
	{
		// Looked up here, not above, because tags aren't loaded yet when the spec is first created.
		Open = FGameplayTag::RequestGameplayTag(TEXT("Room.Exterior.Open"));
		Small = FGameplayTag::RequestGameplayTag(TEXT("Room.Interior.Small"));

		Fixture = MakeUnique<FAtlantisRoomFixture>();
		TestNotNull(TEXT("Test world created"), Fixture->World);
		TestNotNull(TEXT("Test world has a router"), Fixture->Router);
		TestNotNull(TEXT("Test world has a room subsystem"), Fixture->Rooms);
		TestNotNull(TEXT("Stand-in volumes spawned"), Fixture->VolumeB);
	});

	AfterEach([this]()
	{
		Fixture.Reset();
	});

	Describe(TEXT("One volume"), [this]()
	{
		It(TEXT("should report the room when the listener enters a volume"), [this]()
		{
			Fixture->Rooms->EnterRoom(Fixture->VolumeA, Small, 0);

			TestEqual(TEXT("One room fact is sent"), Fixture->Count(), 1);
			TestTrue(TEXT("The room is Small"), Fixture->LastRoom() == Small);
		});

		It(TEXT("should go back to Default when the listener leaves the last volume"), [this]()
		{
			Fixture->Rooms->EnterRoom(Fixture->VolumeA, Small, 0);
			Fixture->Rooms->ExitRoom(Fixture->VolumeA);

			TestEqual(TEXT("Entering and leaving send one fact each"), Fixture->Count(), 2);
			TestTrue(TEXT("The room is Default"), Fixture->LastRoom() == TAG_Room_Default);
		});

		It(TEXT("should send nothing when a volume the listener isn't in is removed"), [this]()
		{
			// Every volume calls ExitRoom when its level ends, including ones the listener never entered.
			Fixture->Rooms->EnterRoom(Fixture->VolumeA, Small, 0);
			Fixture->Rooms->ExitRoom(Fixture->VolumeB);

			TestEqual(TEXT("Only the first entry was sent"), Fixture->Count(), 1);
			TestTrue(TEXT("The room is still Small"), Fixture->LastRoom() == Small);
		});
	});

	Describe(TEXT("Overlapping volumes"), [this]()
	{
		It(TEXT("should pick the higher priority volume"), [this]()
		{
			Fixture->Rooms->EnterRoom(Fixture->VolumeA, Open, 0);
			Fixture->Rooms->EnterRoom(Fixture->VolumeB, Small, 1);
			TestTrue(TEXT("The inner, higher priority room wins"), Fixture->LastRoom() == Small);

			Fixture->Rooms->ExitRoom(Fixture->VolumeB);
			TestTrue(TEXT("Leaving the inner volume returns to the outer room"), Fixture->LastRoom() == Open);
			TestEqual(TEXT("Each change sends one fact"), Fixture->Count(), 3);
		});

		It(TEXT("should keep the higher priority room whichever volume is entered first"), [this]()
		{
			Fixture->Rooms->EnterRoom(Fixture->VolumeB, Small, 1);
			Fixture->Rooms->EnterRoom(Fixture->VolumeA, Open, 0);
			TestEqual(TEXT("Entering the lower priority volume sends nothing"), Fixture->Count(), 1);
			TestTrue(TEXT("The room is still Small"), Fixture->LastRoom() == Small);

			Fixture->Rooms->ExitRoom(Fixture->VolumeB);
			TestTrue(TEXT("The lower priority volume was remembered"), Fixture->LastRoom() == Open);
		});

		It(TEXT("should pick the most recently entered volume on a tie"), [this]()
		{
			Fixture->Rooms->EnterRoom(Fixture->VolumeA, Open, 0);
			Fixture->Rooms->EnterRoom(Fixture->VolumeB, Small, 0);
			TestTrue(TEXT("The newer volume wins"), Fixture->LastRoom() == Small);

			Fixture->Rooms->ExitRoom(Fixture->VolumeB);
			TestTrue(TEXT("Leaving it returns to the older one"), Fixture->LastRoom() == Open);
		});

		It(TEXT("should not repeat a room that hasn't changed"), [this]()
		{
			Fixture->Rooms->EnterRoom(Fixture->VolumeA, Small, 0);
			Fixture->Rooms->EnterRoom(Fixture->VolumeB, Small, 0);
			Fixture->Rooms->ExitRoom(Fixture->VolumeA);

			TestEqual(TEXT("Only the first entry was sent"), Fixture->Count(), 1);
			TestTrue(TEXT("The room is still Small"), Fixture->LastRoom() == Small);
		});
	});

	Describe(TEXT("Room type table"), [this]()
	{
		It(TEXT("should log an error and send nothing for a room missing from the table"), [this]()
		{
			AddExpectedErrorPlain(TEXT("isn't in"), EAutomationExpectedErrorFlags::Contains, 1);

			// Room.Interior exists as the parent of Room.Interior.Small, but has no ID in the table.
			const FGameplayTag Missing = FGameplayTag::RequestGameplayTag(TEXT("Room.Interior"));
			Fixture->Rooms->EnterRoom(Fixture->VolumeA, Missing, 0);

			TestEqual(TEXT("Nothing is sent"), Fixture->Count(), 0);
		});
	});
}

#endif
