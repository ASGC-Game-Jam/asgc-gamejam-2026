// Copyright (c) 2026 ASGC

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/AtlantisPlayerController.h"
#include "UI/AtlantisBallastIndicatorWidget.h"
#include "CommonInputSubsystem.h"
#include "CommonGameViewportClient.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAtlantisBallastInputHintsTest,
	"Atlantis.Input.BallastDeviceHints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAtlantisBallastInputHintsTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>(GEngine));
	GameInstance->InitializeStandalone();
	UWorld* World = GameInstance->GetWorld();
	UCommonGameViewportClient* Viewport = NewObject<UCommonGameViewportClient>(GEngine);
	GameInstance->GetWorldContext()->GameViewport = Viewport;
	Viewport->Init(*GameInstance->GetWorldContext(), GameInstance.Get(), false);
	FString PlayerError;
	ULocalPlayer* LocalPlayer = GameInstance->CreateLocalPlayer(0, PlayerError, false);
	UCommonInputSubsystem* Subsystem = LocalPlayer ? UCommonInputSubsystem::Get(LocalPlayer) : nullptr;
	if (TestNotNull(TEXT("Common Input subsystem created"), Subsystem))
	{
		AAtlantisPlayerController* Controller = World->SpawnActor<AAtlantisPlayerController>();
		Controller->SetPlayer(LocalPlayer);
		// The standalone dummy world does not run actor initialization automatically.
		World->AddController(Controller);
		UClass* WidgetClass = LoadClass<UAtlantisBallastIndicatorWidget>(nullptr,
			TEXT("/Game/ProjectAtlantis/UI/WBP_BallastIndicator.WBP_BallastIndicator_C"));
		Subsystem->SetCurrentInputType(ECommonInputType::Gamepad);
		UAtlantisBallastIndicatorWidget* Widget = WidgetClass
			? CreateWidget<UAtlantisBallastIndicatorWidget>(Controller, WidgetClass) : nullptr;
		if (TestNotNull(TEXT("Ballast widget created"), Widget))
		{
			// Construct the real Slate widget so its native subscription is tested.
			TSharedRef<SWidget> SlateWidget = Widget->TakeWidget();
			TestEqual(TEXT("Widget belongs to the test local player"), Widget->GetOwningLocalPlayer(), LocalPlayer);
			TestTrue(TEXT("Widget subscribed to Common Input"), Subsystem->OnInputMethodChangedNative.IsBoundToObject(Widget));
			UTextBlock* Hints = Cast<UTextBlock>(Widget->GetWidgetFromName(TEXT("BallastStateKeys")));
			if (TestNotNull(TEXT("Hint text exists"), Hints))
			{
				TestTrue(TEXT("Initial hints use Common Input's existing gamepad state"),
					Hints->GetText().ToString().Contains(TEXT("Wander: X")));
				Subsystem->SetCurrentInputType(ECommonInputType::MouseAndKeyboard);
				TestTrue(TEXT("Common Input event switches to keyboard hints"),
					Hints->GetText().ToString().Contains(TEXT("Wander: 3")));
				Subsystem->SetCurrentInputType(ECommonInputType::Gamepad);
				TestTrue(TEXT("Common Input event switches back to Xbox hints"),
					Hints->GetText().ToString().Contains(TEXT("Descend: B")));
			}
			Widget->ReleaseSlateResources(true);
		}
		GameInstance->RemoveLocalPlayer(LocalPlayer);
	}
	GameInstance->Shutdown();
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

#endif