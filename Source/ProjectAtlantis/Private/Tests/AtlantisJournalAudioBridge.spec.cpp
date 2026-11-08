// Copyright (c) 2026 ASGC

#if WITH_DEV_AUTOMATION_TESTS

#include "Audio/AtlantisAudioFactTags.h"
#include "Audio/AtlantisAudioPayloadTypes.h"
#include "Audio/AtlantisJournalAudioBridge.h"
#include "Audio/PubSubSystem/ASGC_DataRouterSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Journal/AtlantisJournalSubsystem.h"
#include "Misc/AutomationTest.h"
#include "StructUtils/InstancedStruct.h"
#include "Tests/AtlantisAudioTestFactListener.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	// A throwaway world with its own router, journal and journal bridge, plus a listener that records investigation facts.
	struct FAtlantisJournalAudioFixture
	{
		TStrongObjectPtr<UGameInstance> GameInstance;
		UWorld* World = nullptr;
		UASGC_DataRouterSubsystem* Router = nullptr;
		UAtlantisJournalSubsystem* Journal = nullptr;
		UAtlantisJournalAudioBridge* Bridge = nullptr;
		TStrongObjectPtr<UAtlantisAudioTestFactListener> Listener;
		int32 SubscriptionHandle = INDEX_NONE;

		FAtlantisJournalAudioFixture()
			: GameInstance(NewObject<UGameInstance>(GEngine))
			, Listener(NewObject<UAtlantisAudioTestFactListener>())
		{
			GameInstance->InitializeStandalone();
			World = GameInstance->GetWorld();
			Journal = GameInstance->GetSubsystem<UAtlantisJournalSubsystem>();
			if (!World)
			{
				return;
			}

			Router = World->GetSubsystem<UASGC_DataRouterSubsystem>();
			Bridge = World->GetSubsystem<UAtlantisJournalAudioBridge>();
			if (Router)
			{
				SubscriptionHandle = Router->Subscribe(TAG_AudioFact_Investigation_Status, Listener.Get());
			}
		}

		~FAtlantisJournalAudioFixture()
		{
			// Unsubscribe first: the router treats a subscriber that vanishes without unsubscribing as a bug.
			if (Router)
			{
				Router->Unsubscribe(TAG_AudioFact_Investigation_Status, SubscriptionHandle);
			}

			GameInstance->Shutdown();
			if (World)
			{
				GEngine->DestroyWorldContext(World);
				World->DestroyWorld(false);
			}
		}

		/** A test world never begins play by itself, so tests start the bridge the way the engine would. */
		void BeginPlay()
		{
			if (Bridge && World)
			{
				Bridge->OnWorldBeginPlay(*World);
			}
		}

		int32 Count() const
		{
			return Listener->ReceivedCount;
		}
	};
}

BEGIN_DEFINE_SPEC(FAtlantisJournalAudioBridgeSpec, "Atlantis.Audio.JournalBridge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	TUniquePtr<FAtlantisJournalAudioFixture> Fixture;

	void TestInvestigationFact(const TCHAR* What, const FName& ExpectedKey, const FGameplayTag& ExpectedState)
	{
		const FAtlantisAudioKeyedStatePayload* Payload = Fixture->Listener->LastPayload.GetPtr<FAtlantisAudioKeyedStatePayload>();
		if (TestNotNull(TEXT("Payload holds a keyed state"), Payload))
		{
			TestEqual(TEXT("The investigation's title is the key"), Payload->Key, ExpectedKey);
			TestTrue(What, Payload->State == ExpectedState);
		}
	}
END_DEFINE_SPEC(FAtlantisJournalAudioBridgeSpec)

void FAtlantisJournalAudioBridgeSpec::Define()
{
	BeforeEach([this]()
	{
		Fixture = MakeUnique<FAtlantisJournalAudioFixture>();
		TestNotNull(TEXT("Test world created"), Fixture->World);
		TestNotNull(TEXT("Test world has a router"), Fixture->Router);
		TestNotNull(TEXT("Journal subsystem exists"), Fixture->Journal);
		TestNotNull(TEXT("Journal bridge exists"), Fixture->Bridge);
	});

	AfterEach([this]()
	{
		Fixture.Reset();
	});

	Describe(TEXT("Starting facts"), [this]()
	{
		It(TEXT("should send the status of an investigation that already exists"), [this]()
		{
			// Update first: this is an investigation that exists before the level starts.
			Fixture->Journal->UpdateInvestigationStatus(TEXT("Existing"), EJournalInvestigationStatus::Inprogress);

			Fixture->BeginPlay();

			TestEqual(TEXT("One investigation fact"), Fixture->Count(), 1);
			TestInvestigationFact(TEXT("Status is in progress"), FName(TEXT("Existing")), TAG_AudioState_Investigation_InProgress);
		});
	});

	Describe(TEXT("Live events"), [this]()
	{
		It(TEXT("should map each investigation status to its state tag"), [this]()
		{
			Fixture->BeginPlay();
			const FName Key(TEXT("Mystery"));

			Fixture->Journal->UpdateInvestigationStatus(TEXT("Mystery"), EJournalInvestigationStatus::Undiscovered);
			TestInvestigationFact(TEXT("Undiscovered maps to its tag"), Key, TAG_AudioState_Investigation_Undiscovered);

			Fixture->Journal->UpdateInvestigationStatus(TEXT("Mystery"), EJournalInvestigationStatus::Inprogress);
			TestInvestigationFact(TEXT("In progress maps to its tag"), Key, TAG_AudioState_Investigation_InProgress);

			Fixture->Journal->UpdateInvestigationStatus(TEXT("Mystery"), EJournalInvestigationStatus::Complete);
			TestInvestigationFact(TEXT("Complete maps to its tag"), Key, TAG_AudioState_Investigation_Complete);

			TestEqual(TEXT("Each change sends one fact"), Fixture->Count(), 3);
		});

		It(TEXT("should reset investigations when the journal clears"), [this]()
		{
			Fixture->BeginPlay();
			const FName Key(TEXT("Mystery"));

			Fixture->Journal->UpdateInvestigationStatus(TEXT("Mystery"), EJournalInvestigationStatus::Inprogress);

			Fixture->Journal->ClearInvestigations();
			TestInvestigationFact(TEXT("Clearing resets the investigation"), Key, TAG_AudioState_Investigation_Undiscovered);

			const int32 CountBefore = Fixture->Count();
			Fixture->Journal->ClearInvestigations();
			TestEqual(TEXT("A second clear sends nothing"), Fixture->Count(), CountBefore);
		});

		// The #489 review case: the journal clears between possessions. No pawn exists in this fixture at all.
		It(TEXT("should report a journal clear while no player is possessed"), [this]()
		{
			Fixture->BeginPlay();

			Fixture->Journal->UpdateInvestigationStatus(TEXT("Mystery"), EJournalInvestigationStatus::Inprogress);
			Fixture->Journal->ClearInvestigations();

			TestInvestigationFact(TEXT("Clearing resets the investigation"), FName(TEXT("Mystery")), TAG_AudioState_Investigation_Undiscovered);
		});
	});

	Describe(TEXT("Binding"), [this]()
	{
		It(TEXT("should stop reporting once deinitialized"), [this]()
		{
			Fixture->BeginPlay();
			Fixture->Bridge->Deinitialize();

			Fixture->Journal->UpdateInvestigationStatus(TEXT("Mystery"), EJournalInvestigationStatus::Inprogress);
			Fixture->Journal->ClearInvestigations();

			TestEqual(TEXT("Nothing is sent"), Fixture->Count(), 0);
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
