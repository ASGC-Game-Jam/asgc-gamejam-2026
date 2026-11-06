// Copyright (c) 2026 ASGC

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/AtlantisPlayerController.h"
#include "Debug/AtlantisCheatManager.h"
#include "Debug/AtlantisDebugMenu.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/Input/SButton.h"

namespace
{
	struct FAtlantisDebugRestartFixture
	{
		TStrongObjectPtr<UWorld> World;
		TStrongObjectPtr<UAtlantisDebugMenu> Menu;
		AAtlantisPlayerController* Controller = nullptr;
		UAtlantisCheatManager* Cheats = nullptr;

		FAtlantisDebugRestartFixture()
			: World(UWorld::CreateWorld(EWorldType::Game, false, TEXT("L_Tutorial_Main"),
				CreatePackage(*FString::Printf(TEXT("/Temp/DebugRestart_%s/L_Tutorial_Main"),
					*FGuid::NewGuid().ToString(EGuidFormats::Digits)))))
		{
			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World.Get());
			Controller = World->SpawnActor<AAtlantisPlayerController>();
			World->AddController(Controller);
			Controller->Player = NewObject<ULocalPlayer>(GEngine);
			Controller->GetLocalPlayer()->PlayerController = Controller;
			Controller->SetAsLocalPlayerController();
			Controller->CheatClass = UCheatManager::StaticClass();
			Controller->EnableCheats();
			Cheats = Cast<UAtlantisCheatManager>(Controller->CheatManager);
		}

		~FAtlantisDebugRestartFixture()
		{
			if (Menu.IsValid())
			{
				Menu->ReleaseSlateResources(true);
				Menu.Reset();
			}
			GEngine->DestroyWorldContext(World.Get());
			World->DestroyWorld(false);
		}

		FWorldContext& Context() const { return *GEngine->GetWorldContextFromWorld(World.Get()); }
	};
}

BEGIN_DEFINE_SPEC(FAtlantisDebugRestartSpec, "Atlantis.Debug.RestartGame",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	TUniquePtr<FAtlantisDebugRestartFixture> Fixture;
END_DEFINE_SPEC(FAtlantisDebugRestartSpec)

void FAtlantisDebugRestartSpec::Define()
{
	BeforeEach([this]()
	{
		Fixture = MakeUnique<FAtlantisDebugRestartFixture>();
		TestNotNull(TEXT("Atlantis Cheat Manager"), Fixture->Cheats);
	});

	AfterEach([this]()
	{
		Fixture.Reset();
	});

	Describe(TEXT("Engine level restart"), [this]()
	{
		It(TEXT("should queue a fresh level restart even without a pawn"), [this]()
		{
			if (!Fixture->Cheats)
			{
				return;
			}
			Fixture->Cheats->RestartGame();
			TestEqual(TEXT("Current level reload requested"), Fixture->Context().TravelURL,
				UGameplayStatics::GetCurrentLevelName(Fixture->World.Get(), true));
			TestTrue(TEXT("Restart clears previous travel options"), Fixture->Context().TravelType == TRAVEL_Absolute);
		});

		It(TEXT("should leave another menu in control"), [this]()
		{
			Fixture->Controller->bShowMouseCursor = true;
			Fixture->Controller->RequestDebugRestart();
			TestTrue(TEXT("No travel while another menu is open"), Fixture->Context().TravelURL.IsEmpty());
		});

		It(TEXT("should not restart while gameplay controls are disabled"), [this]()
		{
			Fixture->Controller->DisableAllControls();
			Fixture->Controller->RequestDebugRestart();
			TestTrue(TEXT("No travel while controls are blocked"), Fixture->Context().TravelURL.IsEmpty());
		});
	});

	Describe(TEXT("Menu action"), [this]()
	{
		It(TEXT("should request the engine restart through button activation"), [this]()
		{
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
			if (!TestEqual(TEXT("Restart and close buttons"), Buttons.Num(), 2))
			{
				return;
			}
			const FKeyEvent Accept(EKeys::Enter, FModifierKeysState(), 0, false, 0, 0);
			Buttons[0]->OnKeyDown(FGeometry(), Accept);
			Buttons[0]->OnKeyUp(FGeometry(), Accept);
			TestEqual(TEXT("Menu reloads current level"), Fixture->Context().TravelURL,
				UGameplayStatics::GetCurrentLevelName(Fixture->World.Get(), true));
		});
	});
}

#endif
