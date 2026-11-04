// Copyright (c) 2026 ASGC

#if WITH_DEV_AUTOMATION_TESTS

#include "Debug/AtlantisDebugInputState.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"

BEGIN_DEFINE_SPEC(FAtlantisDebugInputRoutingSpec, "Atlantis.Debug.InputRouting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	TUniquePtr<FAtlantisDebugInputState> State;
	TStrongObjectPtr<UAtlantisDebugMenuSettings> Settings;
END_DEFINE_SPEC(FAtlantisDebugInputRoutingSpec)

void FAtlantisDebugInputRoutingSpec::Define()
{
	BeforeEach([this]()
	{
		State = MakeUnique<FAtlantisDebugInputState>();
		Settings.Reset(NewObject<UAtlantisDebugMenuSettings>());
		Settings->ToggleMenuKey = EKeys::F7;
		Settings->ToggleMenuGamepadKey = EKeys::Gamepad_Special_Right;
		Settings->ToggleMenuGamepadModifier = EKeys::Gamepad_LeftThumbstick;
		Settings->ResetPlayerKey = EKeys::F10;
		Settings->ResetPlayerGamepadKey = EKeys::Gamepad_RightThumbstick;
	});

	AfterEach([this]()
	{
		State.Reset();
		Settings.Reset();
	});

	Describe(TEXT("Gameplay bindings"), [this]()
	{
		It(TEXT("should leave Menu and right-stick presses available to gameplay without the debug chord"), [this]()
		{
			TestFalse(TEXT("Menu alone passes through"), State->KeyDown(EKeys::Gamepad_Special_Right, false, *Settings, false).bHandled);
			TestFalse(TEXT("Menu release passes through"), State->KeyUp(EKeys::Gamepad_Special_Right));
			TestFalse(TEXT("Right stick outside menu passes through"), State->KeyDown(EKeys::Gamepad_RightThumbstick, false, *Settings, false).bHandled);
			TestFalse(TEXT("Right-stick release passes through"), State->KeyUp(EKeys::Gamepad_RightThumbstick));
		});

		It(TEXT("should consume the toggle press and release only when the modifier is held"), [this]()
		{
			TestFalse(TEXT("Modifier still reaches gameplay"), State->KeyDown(EKeys::Gamepad_LeftThumbstick, false, *Settings, false).bHandled);
			const FAtlantisDebugInputResult Open = State->KeyDown(EKeys::Gamepad_Special_Right, false, *Settings, false);
			TestTrue(TEXT("Chord opens debug menu"), Open.Action == EAtlantisDebugInputAction::ToggleMenu);
			TestTrue(TEXT("Chord press consumed"), Open.bHandled);
			TestTrue(TEXT("Chord release consumed"), State->KeyUp(EKeys::Gamepad_Special_Right));
			TestFalse(TEXT("Modifier release reaches gameplay"), State->KeyUp(EKeys::Gamepad_LeftThumbstick));
		});

		It(TEXT("should leave screenshot input to the screenshot processor"), [this]()
		{
			TestFalse(TEXT("F9 outside menu passes through"), State->KeyDown(EKeys::F9, false, *Settings, false).bHandled);
			State->KeyUp(EKeys::F9);
			TestFalse(TEXT("F9 inside menu passes through"), State->KeyDown(EKeys::F9, false, *Settings, true).bHandled);
		});
	});

	Describe(TEXT("Menu controls"), [this]()
	{
		It(TEXT("should close with Menu without requiring the opening modifier again"), [this]()
		{
			const FAtlantisDebugInputResult Close = State->KeyDown(EKeys::Gamepad_Special_Right, false, *Settings, true);
			TestTrue(TEXT("Menu closes an open menu"), Close.Action == EAtlantisDebugInputAction::ToggleMenu);
			TestTrue(TEXT("Menu press consumed"), Close.bHandled);
			TestTrue(TEXT("Menu release consumed"), State->KeyUp(EKeys::Gamepad_Special_Right));
		});

		It(TEXT("should consume B release after B has closed the menu"), [this]()
		{
			const FAtlantisDebugInputResult Close = State->KeyDown(EKeys::Gamepad_FaceButton_Right, false, *Settings, true);
			TestTrue(TEXT("B closes menu"), Close.Action == EAtlantisDebugInputAction::ToggleMenu);
			TestTrue(TEXT("B press consumed"), Close.bHandled);
			const FAtlantisDebugInputResult Repeat = State->KeyDown(EKeys::Gamepad_FaceButton_Right, true, *Settings, false);
			TestTrue(TEXT("Held B stays consumed after close"), Repeat.bHandled);
			TestTrue(TEXT("Held B cannot reopen"), Repeat.Action == EAtlantisDebugInputAction::None);
			TestTrue(TEXT("Release cannot trigger gameplay"), State->KeyUp(EKeys::Gamepad_FaceButton_Right));
			TestFalse(TEXT("Next normal gameplay B passes through"), State->KeyDown(EKeys::Gamepad_FaceButton_Right, false, *Settings, false).bHandled);
		});

		It(TEXT("should reset with the right stick only while open"), [this]()
		{
			const FAtlantisDebugInputResult Reset = State->KeyDown(EKeys::Gamepad_RightThumbstick, false, *Settings, true);
			TestTrue(TEXT("Menu reset selected"), Reset.Action == EAtlantisDebugInputAction::ResetPlayer);
			TestTrue(TEXT("Menu reset press consumed"), Reset.bHandled);
			TestTrue(TEXT("Menu reset release consumed"), State->KeyUp(EKeys::Gamepad_RightThumbstick));
		});

		It(TEXT("should toggle once per keyboard press"), [this]()
		{
			TestTrue(TEXT("F7 opens"), State->KeyDown(EKeys::F7, false, *Settings, false).Action == EAtlantisDebugInputAction::ToggleMenu);
			TestTrue(TEXT("Repeat is consumed"), State->KeyDown(EKeys::F7, true, *Settings, true).bHandled);
			TestTrue(TEXT("Repeat has no action"), State->KeyDown(EKeys::F7, true, *Settings, true).Action == EAtlantisDebugInputAction::None);
			State->KeyUp(EKeys::F7);
			TestTrue(TEXT("Next F7 closes"), State->KeyDown(EKeys::F7, false, *Settings, true).Action == EAtlantisDebugInputAction::ToggleMenu);
		});
	});
}

#endif
