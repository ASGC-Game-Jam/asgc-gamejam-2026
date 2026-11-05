// Copyright (c) 2026 ASGC

#if WITH_DEV_AUTOMATION_TESTS

#include "AIController.h"
#include "Audio/AtlantisAudioFactBridgeComponent.h"
#include "Audio/AtlantisAudioFactTags.h"
#include "Audio/AtlantisAudioPayloadTypes.h"
#include "Audio/PubSubSystem/ASGC_DataRouterSubsystem.h"
#include "Core/AtlantisHidingComponent.h"
#include "Core/AtlantisOxygenComponent.h"
#include "Core/AtlantisPlayerState.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Journal/AtlantisJournalSubsystem.h"
#include "Misc/AutomationTest.h"
#include "StructUtils/InstancedStruct.h"
#include "Tests/AtlantisAudioTestFactListener.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	// A throwaway game instance and world, so each test gets its own router and journal.
	struct FAtlantisFactBridgeFixture
	{
		struct FSubscription
		{
			TStrongObjectPtr<UAtlantisAudioTestFactListener> Listener;
			int32 Handle = INDEX_NONE;
		};

		TStrongObjectPtr<UGameInstance> GameInstance;
		UWorld* World = nullptr;
		UASGC_DataRouterSubsystem* Router = nullptr;
		UAtlantisJournalSubsystem* Journal = nullptr;
		AAtlantisPlayerState* PlayerState = nullptr;
		UAtlantisOxygenComponent* Oxygen = nullptr;
		AController* Controller = nullptr;
		APawn* Pawn = nullptr;
		UAtlantisHidingComponent* Hiding = nullptr;
		TMap<FGameplayTag, FSubscription> Subscriptions;

		FAtlantisFactBridgeFixture()
			: GameInstance(NewObject<UGameInstance>(GEngine))
		{
			GameInstance->InitializeStandalone();
			World = GameInstance->GetWorld();
			Journal = GameInstance->GetSubsystem<UAtlantisJournalSubsystem>();
			if (!World)
			{
				return;
			}

			// Without this, actors spawned below would never initialize their components.
			World->InitializeActorsForPlay(FURL());
			Router = World->GetSubsystem<UASGC_DataRouterSubsystem>();

			if (Router)
			{
				const FGameplayTag FactTags[] = {
					TAG_AudioFact_Oxygen_Level,
					TAG_AudioFact_Oxygen_Critical,
					TAG_AudioFact_Stealth_Hiding,
					TAG_AudioFact_Investigation_Status};

				for (const FGameplayTag& Tag : FactTags)
				{
					FSubscription& Subscription = Subscriptions.Add(Tag);
					Subscription.Listener.Reset(NewObject<UAtlantisAudioTestFactListener>());
					Subscription.Handle = Router->Subscribe(Tag, Subscription.Listener.Get());
				}
			}

			FActorSpawnParameters SpawnParameters;
			SpawnParameters.ObjectFlags |= RF_Transient;
			PlayerState = World->SpawnActor<AAtlantisPlayerState>(SpawnParameters);
			if (PlayerState)
			{
				Oxygen = PlayerState->GetOxygenComponent();
				PlayerState->DispatchBeginPlay();
			}
			// A player controller only counts as local with a real ULocalPlayer, which this world does not have.
			Controller = World->SpawnActor<AAIController>(SpawnParameters);
			if (Controller)
			{
				Controller->PlayerState = PlayerState;
			}
		}

		~FAtlantisFactBridgeFixture()
		{
			// Unsubscribe first: the router treats a subscriber that vanishes without unsubscribing as a bug.
			if (Router)
			{
				for (const TPair<FGameplayTag, FSubscription>& Subscription : Subscriptions)
				{
					Router->Unsubscribe(Subscription.Key, Subscription.Value.Handle);
				}
			}

			GameInstance->Shutdown();
			if (World)
			{
				GEngine->DestroyWorldContext(World);
				World->DestroyWorld(false);
			}
		}

		/** Spawns a pawn with a bridge on it. The pawn hasn't begun play or been possessed yet. */
		void SpawnPawn(bool bWithHidingComponent = true)
		{
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.ObjectFlags |= RF_Transient;
			Pawn = World->SpawnActor<APawn>(SpawnParameters);
			if (!Pawn)
			{
				return;
			}

			if (bWithHidingComponent)
			{
				Hiding = NewObject<UAtlantisHidingComponent>(Pawn);
				Hiding->RegisterComponent();
			}

			NewObject<UAtlantisAudioFactBridgeComponent>(Pawn)->RegisterComponent();
		}

		/** The usual case: a possessed player, with its starting facts already sent. */
		void StartPlayer()
		{
			SpawnPawn();
			if (Pawn)
			{
				Pawn->PossessedBy(Controller);
				Pawn->DispatchBeginPlay();
			}
		}

		int32 Count(const FGameplayTag& Tag) const
		{
			const FSubscription* Subscription = Subscriptions.Find(Tag);
			return Subscription ? Subscription->Listener->ReceivedCount : 0;
		}

		int32 TotalCount() const
		{
			int32 Total = 0;
			for (const TPair<FGameplayTag, FSubscription>& Subscription : Subscriptions)
			{
				Total += Subscription.Value.Listener->ReceivedCount;
			}
			return Total;
		}

		template <typename PayloadType>
		const PayloadType* Last(const FGameplayTag& Tag) const
		{
			const FSubscription* Subscription = Subscriptions.Find(Tag);
			return Subscription ? Subscription->Listener->LastPayload.GetPtr<PayloadType>() : nullptr;
		}
	};
}

BEGIN_DEFINE_SPEC(FAtlantisAudioFactBridgeSpec, "Atlantis.Audio.FactBridge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	TUniquePtr<FAtlantisFactBridgeFixture> Fixture;

	void TestOxygenLevel(const TCHAR* What, float Expected)
	{
		const FAtlantisAudioFloatPayload* Payload = Fixture->Last<FAtlantisAudioFloatPayload>(TAG_AudioFact_Oxygen_Level);
		if (TestNotNull(TEXT("Oxygen level payload holds a float"), Payload))
		{
			TestEqual(What, Payload->Value, Expected, KINDA_SMALL_NUMBER);
		}
	}

	void TestBoolFact(const TCHAR* What, const FGameplayTag& Tag, bool bExpected)
	{
		const FAtlantisAudioBoolPayload* Payload = Fixture->Last<FAtlantisAudioBoolPayload>(Tag);
		if (TestNotNull(TEXT("Payload holds a bool"), Payload))
		{
			TestEqual(What, Payload->bValue, bExpected);
		}
	}

	void TestInvestigationFact(const TCHAR* What, const FName& ExpectedKey, const FGameplayTag& ExpectedState)
	{
		const FAtlantisAudioKeyedStatePayload* Payload = Fixture->Last<FAtlantisAudioKeyedStatePayload>(TAG_AudioFact_Investigation_Status);
		if (TestNotNull(TEXT("Payload holds a keyed state"), Payload))
		{
			TestEqual(TEXT("The investigation's title is the key"), Payload->Key, ExpectedKey);
			TestTrue(What, Payload->State == ExpectedState);
		}
	}
END_DEFINE_SPEC(FAtlantisAudioFactBridgeSpec)

void FAtlantisAudioFactBridgeSpec::Define()
{
	BeforeEach([this]()
	{
		Fixture = MakeUnique<FAtlantisFactBridgeFixture>();
		TestNotNull(TEXT("Test world created"), Fixture->World);
		TestNotNull(TEXT("Test world has a router"), Fixture->Router);
		TestNotNull(TEXT("Journal subsystem exists"), Fixture->Journal);
		TestNotNull(TEXT("Oxygen component exists"), Fixture->Oxygen);
		TestNotNull(TEXT("Controller spawned"), Fixture->Controller);
	});

	AfterEach([this]()
	{
		Fixture.Reset();
	});

	Describe(TEXT("Starting facts"), [this]()
	{
		It(TEXT("should send each fact's current value once at begin play"), [this]()
		{
			Fixture->StartPlayer();

			TestEqual(TEXT("One oxygen level fact"), Fixture->Count(TAG_AudioFact_Oxygen_Level), 1);
			TestEqual(TEXT("One critical oxygen fact"), Fixture->Count(TAG_AudioFact_Oxygen_Critical), 1);
			TestEqual(TEXT("One hiding fact"), Fixture->Count(TAG_AudioFact_Stealth_Hiding), 1);
			TestOxygenLevel(TEXT("Oxygen starts full"), 1.f);
			TestBoolFact(TEXT("Oxygen doesn't start critical"), TAG_AudioFact_Oxygen_Critical, false);
			TestBoolFact(TEXT("Player doesn't start hiding"), TAG_AudioFact_Stealth_Hiding, false);
		});

		It(TEXT("should send the status of an investigation that already exists"), [this]()
		{
			Fixture->Journal->UpdateInvestigationStatus(TEXT("Existing"), EJournalInvestigationStatus::Inprogress);

			Fixture->StartPlayer();

			TestEqual(TEXT("One investigation fact"), Fixture->Count(TAG_AudioFact_Investigation_Status), 1);
			TestInvestigationFact(TEXT("Status is in progress"), FName(TEXT("Existing")), TAG_AudioState_Investigation_InProgress);
		});
	});

	Describe(TEXT("Oxygen level"), [this]()
	{
		It(TEXT("should report available oxygen as a fraction when oxygen changes"), [this]()
		{
			Fixture->StartPlayer();

			TestTrue(TEXT("Oxygen was consumed"), Fixture->Oxygen->RequestOxygen(25.f));

			TestEqual(TEXT("Change sends one more fact"), Fixture->Count(TAG_AudioFact_Oxygen_Level), 2);
			TestOxygenLevel(TEXT("A quarter of the oxygen is gone"), 0.75f);
		});

		It(TEXT("should exclude oxygen reserved for ballast"), [this]()
		{
			Fixture->StartPlayer();

			TestTrue(TEXT("Oxygen was allocated"), Fixture->Oxygen->RequestOxygenAllocation(50.f));

			TestEqual(TEXT("Allocation sends one more fact"), Fixture->Count(TAG_AudioFact_Oxygen_Level), 2);
			TestOxygenLevel(TEXT("Half of the oxygen is available"), 0.5f);
		});

		It(TEXT("should update when capacity changes"), [this]()
		{
			Fixture->StartPlayer();

			TestTrue(TEXT("Capacity was doubled"), Fixture->Oxygen->SetOxygenCapacity(200.f));

			TestEqual(TEXT("Capacity change sends one more fact"), Fixture->Count(TAG_AudioFact_Oxygen_Level), 2);
			TestOxygenLevel(TEXT("The same oxygen is now half of capacity"), 0.5f);
		});
	});

	Describe(TEXT("Live events"), [this]()
	{
		It(TEXT("should report when oxygen becomes critical"), [this]()
		{
			Fixture->StartPlayer();

			TestTrue(TEXT("Oxygen was consumed"), Fixture->Oxygen->RequestOxygen(Fixture->Oxygen->GetAvailableOxygen() - 1.f));

			TestEqual(TEXT("Change sends one more fact"), Fixture->Count(TAG_AudioFact_Oxygen_Critical), 2);
			TestBoolFact(TEXT("Oxygen is critical"), TAG_AudioFact_Oxygen_Critical, true);
		});

		It(TEXT("should report when hiding changes"), [this]()
		{
			Fixture->StartPlayer();

			Fixture->Hiding->OnHidingStateChanged.Broadcast(true);
			TestEqual(TEXT("Entering sends one more fact"), Fixture->Count(TAG_AudioFact_Stealth_Hiding), 2);
			TestBoolFact(TEXT("Player is hiding"), TAG_AudioFact_Stealth_Hiding, true);

			Fixture->Hiding->OnHidingStateChanged.Broadcast(false);
			TestEqual(TEXT("Leaving sends one more fact"), Fixture->Count(TAG_AudioFact_Stealth_Hiding), 3);
			TestBoolFact(TEXT("Player has stopped hiding"), TAG_AudioFact_Stealth_Hiding, false);
		});

		It(TEXT("should map each investigation status to its state tag"), [this]()
		{
			Fixture->StartPlayer();
			const FName Key(TEXT("Mystery"));

			Fixture->Journal->UpdateInvestigationStatus(TEXT("Mystery"), EJournalInvestigationStatus::Undiscovered);
			TestInvestigationFact(TEXT("Undiscovered maps to its tag"), Key, TAG_AudioState_Investigation_Undiscovered);

			Fixture->Journal->UpdateInvestigationStatus(TEXT("Mystery"), EJournalInvestigationStatus::Inprogress);
			TestInvestigationFact(TEXT("In progress maps to its tag"), Key, TAG_AudioState_Investigation_InProgress);

			Fixture->Journal->UpdateInvestigationStatus(TEXT("Mystery"), EJournalInvestigationStatus::Complete);
			TestInvestigationFact(TEXT("Complete maps to its tag"), Key, TAG_AudioState_Investigation_Complete);

			TestEqual(TEXT("Each change sends one fact"), Fixture->Count(TAG_AudioFact_Investigation_Status), 3);
		});

		It(TEXT("should reset investigations when the journal clears"), [this]()
		{
			Fixture->StartPlayer();
			const FName Key(TEXT("Mystery"));

			Fixture->Journal->UpdateInvestigationStatus(TEXT("Mystery"), EJournalInvestigationStatus::Inprogress);

			Fixture->Journal->ClearInvestigations();
			
			TestInvestigationFact(TEXT("Clearing resets the investigation"), Key, TAG_AudioState_Investigation_Undiscovered);

			const int32 CountBefore = Fixture->Count(TAG_AudioFact_Investigation_Status);
			Fixture->Journal->ClearInvestigations();
			TestEqual(TEXT("A second clear sends nothing"), Fixture->Count(TAG_AudioFact_Investigation_Status), CountBefore);
		});
	});

	Describe(TEXT("Binding"), [this]()
	{
		It(TEXT("should wait for a controller if the pawn begins play first"), [this]()
		{
			Fixture->SpawnPawn();
			Fixture->Pawn->DispatchBeginPlay();
			TestEqual(TEXT("Nothing is sent before possession"), Fixture->TotalCount(), 0);

			Fixture->Pawn->PossessedBy(Fixture->Controller);
			TestEqual(TEXT("Starting facts are sent after possession"), Fixture->Count(TAG_AudioFact_Oxygen_Level), 1);
		});

		It(TEXT("should stop sending facts when unpossessed and resume when possessed again"), [this]()
		{
			Fixture->StartPlayer();

			Fixture->Pawn->UnPossessed();
			const int32 CountBefore = Fixture->TotalCount();

			Fixture->Hiding->OnHidingStateChanged.Broadcast(true);
			Fixture->Oxygen->RequestOxygen(25.f);
			TestEqual(TEXT("Nothing is sent while unpossessed"), Fixture->TotalCount(), CountBefore);

			Fixture->Pawn->PossessedBy(Fixture->Controller);
			TestTrue(TEXT("Facts are sent again after possession"), Fixture->TotalCount() > CountBefore);
		});

		It(TEXT("should stop sending facts after end play"), [this]()
		{
			Fixture->StartPlayer();
			UAtlantisHidingComponent* const Hiding = Fixture->Hiding;
			const int32 CountBefore = Fixture->TotalCount();

			Fixture->Pawn->Destroy();

			Fixture->Oxygen->RequestOxygen(25.f);
			Fixture->Journal->UpdateInvestigationStatus(TEXT("Mystery"), EJournalInvestigationStatus::Inprogress);
			Hiding->OnHidingStateChanged.Broadcast(true);
			TestEqual(TEXT("Nothing more is sent"), Fixture->TotalCount(), CountBefore);
		});
	});

	Describe(TEXT("Missing sources"), [this]()
	{
		It(TEXT("should log the missing hiding component and keep the other facts working"), [this]()
		{
			AddExpectedErrorPlain(TEXT("No hiding component"), EAutomationExpectedErrorFlags::Contains, 1);

			Fixture->SpawnPawn(false);
			Fixture->Pawn->PossessedBy(Fixture->Controller);
			Fixture->Pawn->DispatchBeginPlay();

			TestEqual(TEXT("No hiding fact"), Fixture->Count(TAG_AudioFact_Stealth_Hiding), 0);
			TestEqual(TEXT("Oxygen fact still sent"), Fixture->Count(TAG_AudioFact_Oxygen_Level), 1);
		});

		It(TEXT("should log an error when it isn't on a pawn"), [this]()
		{
			AddExpectedErrorPlain(TEXT("must be on a pawn"), EAutomationExpectedErrorFlags::Contains, 1);

			AActor* const Actor = Fixture->World->SpawnActor<AActor>();
			NewObject<UAtlantisAudioFactBridgeComponent>(Actor)->RegisterComponent();
			Actor->DispatchBeginPlay();

			TestEqual(TEXT("Nothing is sent"), Fixture->TotalCount(), 0);
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
