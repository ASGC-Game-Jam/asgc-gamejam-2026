// Copyright (c) 2026 ASGC

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/AtlantisPlayerController.h"
#include "UI/AtlantisBallastIndicatorWidget.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "InputKeyEventArgs.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAtlantisBallastInputHintsTest,
	"Atlantis.Input.BallastDeviceHints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAtlantisBallastInputHintsTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UWorld> World(UWorld::CreateWorld(EWorldType::Game, false));
	if (!TestNotNull(TEXT("World created"), World.Get()))
	{
		return false;
	}

	AAtlantisPlayerController* Controller = World->SpawnActor<AAtlantisPlayerController>();
	UClass* WidgetClass = LoadClass<UAtlantisBallastIndicatorWidget>(nullptr,
		TEXT("/Game/ProjectAtlantis/UI/WBP_BallastIndicator.WBP_BallastIndicator_C"));
	UAtlantisBallastIndicatorWidget* Widget = WidgetClass
		? CreateWidget<UAtlantisBallastIndicatorWidget>(World.Get(), WidgetClass) : nullptr;
	if (TestNotNull(TEXT("Controller created"), Controller) && TestNotNull(TEXT("Ballast widget created"), Widget))
	{
		UTextBlock* Hints = Cast<UTextBlock>(Widget->GetWidgetFromName(TEXT("BallastStateKeys")));
		if (TestNotNull(TEXT("Hint text exists"), Hints))
		{
			Controller->OnInputDeviceChanged.AddDynamic(Widget, &UAtlantisBallastIndicatorWidget::UpdateInputHints);
			Widget->UpdateInputHints(Controller->IsUsingGamepad());
			TestTrue(TEXT("Initial hints show keyboard"), Hints->GetText().ToString().Contains(TEXT("Descend: 2")));
			Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_FaceButton_Left, IE_Pressed, 1.f));
			TestTrue(TEXT("Gamepad press selects gamepad"), Controller->IsUsingGamepad());
			TestTrue(TEXT("Gamepad event updates widget"), Hints->GetText().ToString().Contains(TEXT("Wander: X")));
			Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::W, IE_Released, 0.f));
			TestTrue(TEXT("Old keyboard release does not switch hints"), Controller->IsUsingGamepad());
			Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::W, IE_Pressed, 1.f));
			TestFalse(TEXT("Keyboard press switches back"), Controller->IsUsingGamepad());
			TestTrue(TEXT("Keyboard event updates widget"), Hints->GetText().ToString().Contains(TEXT("Wander: 3")));
			Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_LeftX, IE_Axis, 0.05f));
			TestFalse(TEXT("Stick drift does not switch hints"), Controller->IsUsingGamepad());
			Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_LeftX, IE_Axis, 0.5f));
			TestTrue(TEXT("Deliberate stick movement selects gamepad"), Controller->IsUsingGamepad());
			Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::MouseX, IE_Axis, 4.f));
			TestFalse(TEXT("Mouse movement switches back"), Controller->IsUsingGamepad());
			Controller->OnInputDeviceChanged.RemoveDynamic(Widget, &UAtlantisBallastIndicatorWidget::UpdateInputHints);
		}
	}
	World->DestroyWorld(false);
	return true;
}

#endif