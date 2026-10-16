// Copyright (c) 2026 ASGC

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/AtlantisPlayerCharacter.h"
#include "Core/AtlantisPlayerController.h"
#include "Debug/AtlantisCheatManager.h"
#include "Debug/AtlantisDebugMenu.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/Input/SButton.h"

namespace
{
	struct FAtlantisDebugResetFixture
	{
		TStrongObjectPtr<UWorld> World;
		TStrongObjectPtr<UAtlantisDebugMenu> Menu;
		AAtlantisPlayerCharacter* Character = nullptr;
		AAtlantisPlayerController* Controller = nullptr;
		UAtlantisCheatManager* Cheats = nullptr;
		const FVector Start = FVector(100.f, 200.f, 300.f);
		const FRotator Facing = FRotator(0.f, 45.f, 0.f);

		FAtlantisDebugResetFixture()
			: World(UWorld::CreateWorld(EWorldType::Game, false))
		{
			if (!World.IsValid())
			{
				return;
			}
			Character = World->SpawnActor<AAtlantisPlayerCharacter>(Start, Facing);
			Controller = World->SpawnActor<AAtlantisPlayerController>();
			if (!Character || !Controller)
			{
				return;
			}
			Character->DispatchBeginPlay();
			World->AddController(Controller);
			Controller->Player = NewObject<ULocalPlayer>(GEngine);
			Controller->GetLocalPlayer()->PlayerController = Controller;
			Controller->SetAsLocalPlayerController();
			Controller->Possess(Character);
			// Existing Blueprint defaults must still select our Cheat Manager.
			Controller->CheatClass = UCheatManager::StaticClass();
			Controller->EnableCheats();
			Cheats = Cast<UAtlantisCheatManager>(Controller->CheatManager);
		}

		~FAtlantisDebugResetFixture()
		{
			if (Menu.IsValid())
			{
				Menu->ReleaseSlateResources(true);
				Menu.Reset();
			}
			if (World.IsValid())
			{
				World->DestroyWorld(false);
			}
		}

		bool IsReady() const { return Character && Controller && Cheats; }

		void MoveAway()
		{
			Character->SetActorLocation(Start + FVector(1000.f, 0.f, 0.f));
			Controller->SetControlRotation(FRotator(20.f, 180.f, 0.f));
			Character->GetCharacterMovement()->Velocity = FVector(200.f, 300.f, -400.f);
			Character->GetCharacterMovement()->AddInputVector(FVector::ForwardVector);
		}
	};
}

BEGIN_DEFINE_SPEC(FAtlantisDebugResetSpec, "Atlantis.Debug.ResetPlayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	TUniquePtr<FAtlantisDebugResetFixture> Fixture;
END_DEFINE_SPEC(FAtlantisDebugResetSpec)

void FAtlantisDebugResetSpec::Define()
{
	BeforeEach([this]()
	{
		Fixture = MakeUnique<FAtlantisDebugResetFixture>();
		TestTrue(TEXT("Local character and upgraded Cheat Manager are ready"), Fixture->IsReady());
	});

	AfterEach([this]()
	{
		Fixture.Reset();
	});

	Describe(TEXT("Reset command"), [this]()
	{
		It(TEXT("should return to the initial position and camera facing"), [this]()
		{
			if (!Fixture->IsReady())
			{
				return;
			}
			Fixture->MoveAway();
			Fixture->Cheats->ResetPlayer();
			TestTrue(TEXT("Initial position restored"), Fixture->Character->GetActorLocation().Equals(Fixture->Start, 1.f));
			TestTrue(TEXT("Initial facing restored"), Fixture->Controller->GetControlRotation().Equals(Fixture->Facing, 0.1f));
		});

		It(TEXT("should clear residual velocity and pending input"), [this]()
		{
			if (!Fixture->IsReady())
			{
				return;
			}
			Fixture->MoveAway();
			Fixture->Cheats->ResetPlayer();
			TestTrue(TEXT("Velocity cleared"), Fixture->Character->GetCharacterMovement()->Velocity.IsNearlyZero());
			TestTrue(TEXT("Input cleared"), Fixture->Character->GetPendingMovementInputVector().IsNearlyZero());
		});

		It(TEXT("should remain stable when repeated"), [this]()
		{
			if (!Fixture->IsReady())
			{
				return;
			}
			Fixture->MoveAway();
			Fixture->Cheats->ResetPlayer();
			Fixture->Cheats->ResetPlayer();
			TestTrue(TEXT("Repeated reset stays at start"), Fixture->Character->GetActorLocation().Equals(Fixture->Start, 1.f));
		});

		It(TEXT("should safely ignore a controller without a pawn"), [this]()
		{
			if (!Fixture->IsReady())
			{
				return;
			}
			Fixture->Controller->UnPossess();
			Fixture->Cheats->ResetPlayer();
			TestNull(TEXT("No pawn remains safe"), Fixture->Controller->GetPawn());
		});
	});

	Describe(TEXT("Native menu"), [this]()
	{
		It(TEXT("should execute the reset command through button keyboard activation"), [this]()
		{
			if (!Fixture->IsReady())
			{
				return;
			}
			Fixture->Menu.Reset(CreateWidget<UAtlantisDebugMenu>(Fixture->Controller));
			if (!TestNotNull(TEXT("Menu created"), Fixture->Menu.Get()))
			{
				return;
			}
			TArray<TSharedRef<SButton>> Buttons;
			TFunction<void(TSharedRef<SWidget>)> FindButtons = [&FindButtons, &Buttons](TSharedRef<SWidget> Widget)
			{
				if (Widget->GetType() == FName(TEXT("SButton")))
				{
					Buttons.Add(StaticCastSharedRef<SButton>(Widget));
				}
				FChildren* Children = Widget->GetChildren();
				for (int32 Index = 0; Index < Children->Num(); ++Index)
				{
					FindButtons(Children->GetChildAt(Index));
				}
			};
			FindButtons(Fixture->Menu->TakeWidget());
			if (!TestEqual(TEXT("Reset and close buttons"), Buttons.Num(), 2))
			{
				return;
			}
			Fixture->MoveAway();
			const FKeyEvent Accept(EKeys::Enter, FModifierKeysState(), 0, false, 0, 0);
			Buttons[0]->OnKeyDown(FGeometry(), Accept);
			Buttons[0]->OnKeyUp(FGeometry(), Accept);
			TestTrue(TEXT("Button returns character to start"), Fixture->Character->GetActorLocation().Equals(Fixture->Start, 1.f));
		});
	});
}

#endif
