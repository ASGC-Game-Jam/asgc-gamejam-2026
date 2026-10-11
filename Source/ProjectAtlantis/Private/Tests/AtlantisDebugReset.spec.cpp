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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAtlantisDebugResetTest, "Atlantis.Debug.ResetPlayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAtlantisDebugResetTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UWorld> World(UWorld::CreateWorld(EWorldType::Game, false));
	if (!TestNotNull(TEXT("World created"), World.Get()))
	{
		return false;
	}
	const FVector Start(100.f, 200.f, 300.f);
	const FRotator Facing(0.f, 45.f, 0.f);
	AAtlantisPlayerCharacter* Character = World->SpawnActor<AAtlantisPlayerCharacter>(Start, Facing);
	AAtlantisPlayerController* Controller = World->SpawnActor<AAtlantisPlayerController>();
	if (!TestNotNull(TEXT("Character spawned"), Character) ||
		!TestNotNull(TEXT("Controller spawned"), Controller))
	{
		World->DestroyWorld(false);
		return false;
	}
	Character->DispatchBeginPlay();
	World->AddController(Controller);
	Controller->Player = NewObject<ULocalPlayer>(GEngine);
	Controller->GetLocalPlayer()->PlayerController = Controller;
	Controller->Possess(Character);
	TestTrue(TEXT("Controller possesses test pawn"), Controller->GetPawn() == Character);
	TestTrue(TEXT("Controller is local"), Controller->IsLocalController());
	Character->SetActorLocation(Start + FVector(1000.f, 0.f, 0.f));
	Controller->SetControlRotation(FRotator(20.f, 180.f, 0.f));
	UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	Movement->Velocity = FVector(200.f, 300.f, -400.f);
	Movement->AddInputVector(FVector::ForwardVector);
	// Exercise the console command's manager path, including the authority RPC.
	Controller->CheatClass = UCheatManager::StaticClass();
	Controller->EnableCheats();
	UAtlantisCheatManager* Cheats = Cast<UAtlantisCheatManager>(Controller->CheatManager);
	if (!TestNotNull(TEXT("Existing Blueprint default upgrades to Atlantis cheats"), Cheats))
	{
		World->DestroyWorld(false);
		return false;
	}
	Cheats->ResetPlayer();
	TestTrue(TEXT("Returns to initial position"), Character->GetActorLocation().Equals(Start, 1.f));
	TestTrue(TEXT("Restores initial facing"), Controller->GetControlRotation().Equals(Facing, 0.1f));
	TestTrue(TEXT("Clears velocity"), Movement->Velocity.IsNearlyZero());
	TestTrue(TEXT("Clears pending movement"), Character->GetPendingMovementInputVector().IsNearlyZero());
	Cheats->ResetPlayer();
	TestTrue(TEXT("Repeated reset is stable"), Character->GetActorLocation().Equals(Start, 1.f));
	// Exercise the real native menu and standard Slate button accept path.
	TStrongObjectPtr<UAtlantisDebugMenu> Menu(CreateWidget<UAtlantisDebugMenu>(Controller));
	if (TestNotNull(TEXT("Menu created for local player"), Menu.Get()))
	{
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
		FindButtons(Menu->TakeWidget());
		TestEqual(TEXT("Menu has reset and close buttons"), Buttons.Num(), 2);
		if (Buttons.Num() == 2)
		{
			Character->SetActorLocation(Start + FVector(500.f, 0.f, 0.f));
			const FKeyEvent Accept(EKeys::Enter, FModifierKeysState(), 0, false, 0, 0);
			Buttons[0]->OnKeyDown(FGeometry(), Accept);
			Buttons[0]->OnKeyUp(FGeometry(), Accept);
			TestTrue(TEXT("Menu button executes reset command"), Character->GetActorLocation().Equals(Start, 1.f));
		}
		Menu->ReleaseSlateResources(true);
	}
	Controller->UnPossess();
	Cheats->ResetPlayer();
	TestNull(TEXT("No pawn reset is safe"), Controller->GetPawn());
	World->DestroyWorld(false);
	return true;
}

#endif
