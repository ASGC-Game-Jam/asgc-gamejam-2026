// Copyright (c) 2026 ASGC

#if WITH_DEV_AUTOMATION_TESTS

#include "Interaction/AtlantisInteractableComponent.h"
#include "Interaction/AtlantisInteractorComponent.h"
#include "Tests/AtlantisInteractionTestListener.h"
#include "AIController.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/Material.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	constexpr float TestInteractionRange = 300.f;
	constexpr float TestTraceRadius = 15.f;

	// Each test owns a minimal runtime world, independent of the editor's current map.
	struct FAtlantisInteractionFixture
	{
		TStrongObjectPtr<UWorld> World;
		TStrongObjectPtr<UAtlantisInteractionTestListener> Listener;
		APawn* Player = nullptr;
		UAtlantisInteractorComponent* Interactor = nullptr;

		FAtlantisInteractionFixture()
			: World(UWorld::CreateWorld(EWorldType::Game, false)),
			  Listener(NewObject<UAtlantisInteractionTestListener>())
		{
			if (!World.IsValid())
			{
				return;
			}

			// The player sits at the origin looking down +X.
			Player = SpawnControlledPawn<APlayerController>(FVector::ZeroVector);
			Interactor = AddInteractor(Player);
			Interactor->OnFocusChanged.AddDynamic(Listener.Get(), &UAtlantisInteractionTestListener::RecordFocusChanged);
		}

		~FAtlantisInteractionFixture()
		{
			if (World.IsValid())
			{
				World->DestroyWorld(false);
			}
		}

		template <typename ActorType = AActor>
		ActorType* SpawnActorWithRoot(const FVector& Location) const
		{
			FActorSpawnParameters Params;
			Params.ObjectFlags |= RF_Transient;
			ActorType* Actor = World->SpawnActor<ActorType>(Params);
			USceneComponent* Root = NewObject<USceneComponent>(Actor);
			Actor->SetRootComponent(Root);
			Root->RegisterComponent();
			Actor->SetActorLocation(Location);
			return Actor;
		}

		/** A pawn facing +X with its eyes at its origin, possessed by a new ControllerType. */
		template <typename ControllerType>
		APawn* SpawnControlledPawn(const FVector& Location) const
		{
			APawn* Pawn = SpawnActorWithRoot<APawn>(Location);
			Pawn->BaseEyeHeight = 0.f;
			FActorSpawnParameters Params;
			Params.ObjectFlags |= RF_Transient;
			ControllerType* Controller = World->SpawnActor<ControllerType>(Params);
			if constexpr (std::is_base_of_v<APlayerController, ControllerType>)
			{
				// Without a net driver the engine treats a PlayerController as local only if it owns a
				// ULocalPlayer, which the real player always has. Give the test players one too.
				Controller->Player = NewObject<ULocalPlayer>(GEngine);
			}
			Controller->Possess(Pawn);
			return Pawn;
		}

		/** Makes the pawn look down -X, whichever of pawn/controller the view point is taken from. */
		static void TurnAround(APawn* Pawn)
		{
			const FRotator Behind(0.f, 180.f, 0.f);
			Pawn->SetActorRotation(Behind);
			if (AController* Controller = Pawn->GetController())
			{
				Controller->SetControlRotation(Behind);
				// AController hides SetActorRotation; the view point may still read the controller's own rotation.
				static_cast<AActor*>(Controller)->SetActorRotation(Behind);
			}
		}

		UAtlantisInteractorComponent* AddInteractor(APawn* Pawn) const
		{
			// The C++ class has no defaults (they live in the Blueprint), so the test configures it.
			UAtlantisInteractorComponent* NewInteractor = NewObject<UAtlantisInteractorComponent>(Pawn);
			NewInteractor->InteractionRange = TestInteractionRange;
			NewInteractor->TraceRadius = TestTraceRadius;
			NewInteractor->TraceChannel = ECC_Visibility; // test cubes use BlockAll, which blocks every channel
			NewInteractor->RegisterComponent();
			return NewInteractor;
		}

		/** A cube that blocks every trace channel, so also the Interact channel. */
		UStaticMeshComponent* AddBlockingCube(AActor* Actor) const
		{
			UStaticMeshComponent* MeshComponent = NewObject<UStaticMeshComponent>(Actor);
			MeshComponent->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
			MeshComponent->SetCollisionProfileName(TEXT("BlockAll"));
			MeshComponent->SetupAttachment(Actor->GetRootComponent());
			MeshComponent->RegisterComponent();
			return MeshComponent;
		}

		UAtlantisInteractableComponent* SpawnInteractable(const FVector& Location, UStaticMeshComponent** OutMesh = nullptr) const
		{
			AActor* Actor = SpawnActorWithRoot(Location);
			UStaticMeshComponent* MeshComponent = AddBlockingCube(Actor);
			if (OutMesh)
			{
				*OutMesh = MeshComponent;
			}

			UAtlantisInteractableComponent* Interactable = NewObject<UAtlantisInteractableComponent>(Actor);
			Interactable->HighlightMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
			Interactable->RegisterComponent();
			Interactable->OnInteracted.AddDynamic(Listener.Get(), &UAtlantisInteractionTestListener::RecordInteracted);
			Interactable->OnFocused.AddDynamic(Listener.Get(), &UAtlantisInteractionTestListener::RecordFocused);
			Interactable->OnUnfocused.AddDynamic(Listener.Get(), &UAtlantisInteractionTestListener::RecordUnfocused);
			return Interactable;
		}
	};
}

BEGIN_DEFINE_SPEC(FAtlantisInteractionSpec, "Atlantis.Interaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	TUniquePtr<FAtlantisInteractionFixture> Fixture;
END_DEFINE_SPEC(FAtlantisInteractionSpec)

void FAtlantisInteractionSpec::Define()
{
	BeforeEach([this]()
	{
		Fixture = MakeUnique<FAtlantisInteractionFixture>();
		TestNotNull(TEXT("Test world created"), Fixture->World.Get());
		TestNotNull(TEXT("Interactor created"), Fixture->Interactor);
	});

	AfterEach([this]()
	{
		Fixture.Reset();
	});

	Describe("Interactable", [this]()
	{
		It("broadcasts OnInteracted with the interactor", [this]()
		{
			UAtlantisInteractableComponent* Interactable = Fixture->SpawnInteractable(FVector(1000.f, 0.f, 0.f));
			TestTrue(TEXT("Interaction happened"), Interactable->TryInteract(Fixture->Player));
			TestEqual(TEXT("Broadcast once"), Fixture->Listener->InteractedCount, 1);
			TestTrue(TEXT("Interactor passed through"), Fixture->Listener->LastInteractor.Get() == Fixture->Player);
		});

		It("ignores interaction while disabled", [this]()
		{
			UAtlantisInteractableComponent* Interactable = Fixture->SpawnInteractable(FVector(1000.f, 0.f, 0.f));
			Interactable->SetInteractionEnabled(false);
			TestFalse(TEXT("Interaction refused"), Interactable->TryInteract(Fixture->Player));
			TestEqual(TEXT("No broadcast"), Fixture->Listener->InteractedCount, 0);
		});

		It("disables itself after one use when single use", [this]()
		{
			UAtlantisInteractableComponent* Interactable = Fixture->SpawnInteractable(FVector(1000.f, 0.f, 0.f));
			Interactable->bSingleUse = true;
			TestTrue(TEXT("First use works"), Interactable->TryInteract(Fixture->Player));
			TestFalse(TEXT("Second use refused"), Interactable->TryInteract(Fixture->Player));
			TestFalse(TEXT("Now disabled"), Interactable->CanInteract());
			TestEqual(TEXT("Broadcast once"), Fixture->Listener->InteractedCount, 1);
		});

		It("highlights while focused and restores the previous overlay", [this]()
		{
			UStaticMeshComponent* MeshComponent = nullptr;
			UAtlantisInteractableComponent* Interactable = Fixture->SpawnInteractable(FVector(200.f, 0.f, 0.f), &MeshComponent);
			UMaterialInterface* Previous = UMaterial::GetDefaultMaterial(MD_PostProcess);
			MeshComponent->SetOverlayMaterial(Previous);

			Fixture->Interactor->RefreshFocus();
			TestTrue(TEXT("Highlight applied"), MeshComponent->GetOverlayMaterial() == Interactable->HighlightMaterial);
			TestTrue(TEXT("Reports focused"), Interactable->IsFocused());
			TestTrue(TEXT("Focused by the player"), Interactable->IsFocusedBy(Fixture->Player));

			FAtlantisInteractionFixture::TurnAround(Fixture->Player);
			Fixture->Interactor->RefreshFocus();
			TestTrue(TEXT("Previous overlay restored"), MeshComponent->GetOverlayMaterial() == Previous);
			TestFalse(TEXT("No longer focused"), Interactable->IsFocused());
		});
	});

	Describe("Interactor", [this]()
	{
		It("focuses and highlights an interactable in front within range", [this]()
		{
			UStaticMeshComponent* MeshComponent = nullptr;
			UAtlantisInteractableComponent* Interactable = Fixture->SpawnInteractable(FVector(200.f, 0.f, 0.f), &MeshComponent);
			Fixture->Interactor->RefreshFocus();
			TestTrue(TEXT("Focused"), Fixture->Interactor->GetFocusedInteractable() == Interactable);
			TestEqual(TEXT("OnFocused fired"), Fixture->Listener->FocusedCount, 1);
			TestEqual(TEXT("OnFocusChanged fired"), Fixture->Listener->FocusChangedCount, 1);
			TestTrue(TEXT("Highlight applied"), MeshComponent->GetOverlayMaterial() == Interactable->HighlightMaterial);
		});

		It("interacts with the focused interactable", [this]()
		{
			Fixture->SpawnInteractable(FVector(200.f, 0.f, 0.f));
			TestTrue(TEXT("Interaction happened"), Fixture->Interactor->Interact());
			TestEqual(TEXT("OnInteracted fired"), Fixture->Listener->InteractedCount, 1);
			TestTrue(TEXT("Player is the interactor"), Fixture->Listener->LastInteractor.Get() == Fixture->Player);
		});

		It("ignores interactables out of range", [this]()
		{
			Fixture->SpawnInteractable(FVector(TestInteractionRange * 3.f, 0.f, 0.f));
			Fixture->Interactor->RefreshFocus();
			TestNull(TEXT("Nothing focused"), Fixture->Interactor->GetFocusedInteractable());
			TestFalse(TEXT("No interaction"), Fixture->Interactor->Interact());
		});

		It("ignores interactables behind a wall", [this]()
		{
			AActor* Wall = Fixture->SpawnActorWithRoot(FVector(120.f, 0.f, 0.f));
			Fixture->AddBlockingCube(Wall);
			Fixture->SpawnInteractable(FVector(250.f, 0.f, 0.f));
			Fixture->Interactor->RefreshFocus();
			TestNull(TEXT("Nothing focused"), Fixture->Interactor->GetFocusedInteractable());
		});

		It("drops focus when the focused interactable gets disabled", [this]()
		{
			UAtlantisInteractableComponent* Interactable = Fixture->SpawnInteractable(FVector(200.f, 0.f, 0.f));
			Fixture->Interactor->RefreshFocus();
			Interactable->SetInteractionEnabled(false);
			TestEqual(TEXT("OnUnfocused fired"), Fixture->Listener->UnfocusedCount, 1);
			Fixture->Interactor->RefreshFocus();
			TestNull(TEXT("Nothing focused"), Fixture->Interactor->GetFocusedInteractable());
			TestEqual(TEXT("Focus cleared once more"), Fixture->Listener->FocusChangedCount, 2);
			TestEqual(TEXT("OnUnfocused not fired twice"), Fixture->Listener->UnfocusedCount, 1);
		});

		It("keeps the highlight until every player looks away", [this]()
		{
			UStaticMeshComponent* MeshComponent = nullptr;
			UAtlantisInteractableComponent* Interactable = Fixture->SpawnInteractable(FVector(200.f, 0.f, 0.f), &MeshComponent);
			APawn* SecondPlayer = Fixture->SpawnControlledPawn<APlayerController>(FVector::ZeroVector);
			UAtlantisInteractorComponent* SecondInteractor = Fixture->AddInteractor(SecondPlayer);

			Fixture->Interactor->RefreshFocus();
			SecondInteractor->RefreshFocus();
			TestEqual(TEXT("Both players counted"), Fixture->Listener->FocusedCount, 2);

			FAtlantisInteractionFixture::TurnAround(SecondPlayer);
			SecondInteractor->RefreshFocus();
			TestNull(TEXT("Second player looks away"), SecondInteractor->GetFocusedInteractable());
			TestTrue(TEXT("Still highlighted for the first player"), MeshComponent->GetOverlayMaterial() == Interactable->HighlightMaterial);
			TestTrue(TEXT("Still focused"), Interactable->IsFocused());

			FAtlantisInteractionFixture::TurnAround(Fixture->Player);
			Fixture->Interactor->RefreshFocus();
			TestNull(TEXT("Highlight removed once nobody looks"), MeshComponent->GetOverlayMaterial());
		});
	});

	Describe("AI", [this]()
	{
		It("does not highlight what an AI interactor looks at", [this]()
		{
			// AI pawn off to the side, looking at an interactable the player isn't facing.
			APawn* Enemy = Fixture->SpawnControlledPawn<AAIController>(FVector(0.f, 1000.f, 0.f));
			UAtlantisInteractorComponent* EnemyInteractor = Fixture->AddInteractor(Enemy);
			UStaticMeshComponent* MeshComponent = nullptr;
			UAtlantisInteractableComponent* Interactable = Fixture->SpawnInteractable(FVector(200.f, 1000.f, 0.f), &MeshComponent);

			EnemyInteractor->RefreshFocus();
			TestTrue(TEXT("AI still knows what it's looking at"), EnemyInteractor->GetFocusedInteractable() == Interactable);
			TestNull(TEXT("No highlight"), MeshComponent->GetOverlayMaterial());
			TestFalse(TEXT("Not reported as focused"), Interactable->IsFocused());
			TestEqual(TEXT("OnFocused not fired"), Fixture->Listener->FocusedCount, 0);

			TestTrue(TEXT("AI can still interact"), EnemyInteractor->Interact());
			TestTrue(TEXT("Enemy is the interactor"), Fixture->Listener->LastInteractor.Get() == Enemy);
		});

		It("can use an interactable directly without looking at it", [this]()
		{
			APawn* Enemy = Fixture->SpawnControlledPawn<AAIController>(FVector(0.f, 1000.f, 0.f));
			UStaticMeshComponent* MeshComponent = nullptr;
			UAtlantisInteractableComponent* Interactable = Fixture->SpawnInteractable(FVector(-500.f, -500.f, 0.f), &MeshComponent);

			TestTrue(TEXT("Interaction happened"), Interactable->TryInteract(Enemy));
			TestTrue(TEXT("Enemy is the interactor"), Fixture->Listener->LastInteractor.Get() == Enemy);
			TestNull(TEXT("No highlight"), MeshComponent->GetOverlayMaterial());
		});
	});
}

#endif
