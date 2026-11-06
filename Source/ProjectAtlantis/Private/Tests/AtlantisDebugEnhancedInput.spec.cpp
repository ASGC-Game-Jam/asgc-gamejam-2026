// Copyright (c) 2026 ASGC

#if WITH_DEV_AUTOMATION_TESTS

#include "Debug/AtlantisDebugMenuSettings.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputMappingContext.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"

BEGIN_DEFINE_SPEC(FAtlantisDebugEnhancedInputSpec, "Atlantis.Debug.EnhancedInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	TStrongObjectPtr<UWorld> World;
	TStrongObjectPtr<UAtlantisDebugMenuSettings> Settings;
	TStrongObjectPtr<UEnhancedInputLocalPlayerSubsystem> Subsystem;
	APlayerController* Controller = nullptr;
	UEnhancedPlayerInput* Input = nullptr;
	FAtlantisDebugInputMappings Mappings;
	void Key(const FKey& Key, EInputEvent Event);
	ETriggerEvent EventFor(UInputAction* Action) const;
	void UseContext(UInputMappingContext* Context);
END_DEFINE_SPEC(FAtlantisDebugEnhancedInputSpec)

void FAtlantisDebugEnhancedInputSpec::Key(const FKey& InputKey, EInputEvent Event)
{
	Input->InputKey(FInputKeyEventArgs::CreateSimulated(InputKey, Event,
		Event == IE_Released ? 0.f : 1.f, 0, FInputDeviceId::CreateFromInternalId(0)));
	TArray<UInputComponent*> Stack;
	Input->ProcessInputStack(Stack, 1.f / 60.f, false);
}

ETriggerEvent FAtlantisDebugEnhancedInputSpec::EventFor(UInputAction* Action) const
{
	const FInputActionInstance* Instance = Input->FindActionInstanceData(Action);
	return Instance ? Instance->GetTriggerEvent() : ETriggerEvent::None;
}

void FAtlantisDebugEnhancedInputSpec::UseContext(UInputMappingContext* Context)
{
	FModifyContextOptions Options;
	Options.bForceImmediately = true;
	Options.bIgnoreAllPressedKeysUntilRelease = true;
	Subsystem->RemoveMappingContext(Mappings.Gameplay, Options);
	Subsystem->RemoveMappingContext(Mappings.Menu, Options);
	Subsystem->AddMappingContext(Context, 1000, Options);
}

void FAtlantisDebugEnhancedInputSpec::Define()
{
	BeforeEach([this]()
	{
		World.Reset(UWorld::CreateWorld(EWorldType::Game, false));
		Controller = World->SpawnActor<APlayerController>();
		ULocalPlayer* Player = NewObject<ULocalPlayer>(GEngine);
		Player->PlayerController = Controller;
		Controller->Player = Player;
		Controller->SetAsLocalPlayerController();
		Input = NewObject<UEnhancedPlayerInput>(Controller);
		Controller->PlayerInput = Input;
		Subsystem.Reset(NewObject<UEnhancedInputLocalPlayerSubsystem>(Player));
		Settings.Reset(NewObject<UAtlantisDebugMenuSettings>());
		Settings->ToggleMenuKey = EKeys::F7;
		Settings->ToggleMenuGamepadKey = EKeys::Gamepad_Special_Right;
		Settings->ToggleMenuGamepadModifier = EKeys::Gamepad_LeftThumbstick;
		Settings->ResetPlayerKey = EKeys::F10;
		Settings->ResetPlayerGamepadKey = EKeys::Gamepad_RightThumbstick;
		Mappings = Settings->CreateInputMappings(Controller);
		UseContext(Mappings.Gameplay);
	});

	AfterEach([this]()
	{
		Subsystem->RemoveMappingContext(Mappings.Gameplay);
		Subsystem->RemoveMappingContext(Mappings.Menu);
		Subsystem.Reset();
		Settings.Reset();
		World->DestroyWorld(false);
		World.Reset();
	});

	Describe(TEXT("Context transitions"), [this]()
	{
		It(TEXT("should keep keyboard and gamepad toggles working across a menu transition"), [this]()
		{
			Key(EKeys::Gamepad_Special_Right, IE_Pressed);
			TestTrue(TEXT("Menu alone cannot open"), EventFor(Mappings.GamepadToggle) != ETriggerEvent::Triggered);
			Key(EKeys::Gamepad_Special_Right, IE_Released);
			Key(EKeys::Gamepad_LeftThumbstick, IE_Pressed);
			Key(EKeys::Gamepad_Special_Right, IE_Pressed);
			TestTrue(TEXT("Native chord triggers open"), EventFor(Mappings.GamepadToggle) == ETriggerEvent::Triggered);
			UseContext(Mappings.Menu);
			Key(EKeys::Gamepad_Special_Right, IE_Repeat);
			TestTrue(TEXT("Held opening key cannot close"), EventFor(Mappings.GamepadToggle) != ETriggerEvent::Triggered);
			Key(EKeys::Gamepad_Special_Right, IE_Released);
			Key(EKeys::Gamepad_LeftThumbstick, IE_Released);
			Key(EKeys::Gamepad_RightThumbstick, IE_Pressed);
			TestTrue(TEXT("Menu stick reset triggers"), EventFor(Mappings.Reset) == ETriggerEvent::Triggered);
			Key(EKeys::Gamepad_RightThumbstick, IE_Released);
			Key(EKeys::Gamepad_FaceButton_Right, IE_Pressed);
			TestTrue(TEXT("B closes through its action"), EventFor(Mappings.Close) == ETriggerEvent::Triggered);
			UseContext(Mappings.Gameplay);
			Key(EKeys::Gamepad_FaceButton_Right, IE_Released);
			Key(EKeys::F7, IE_Pressed);
			TestTrue(TEXT("Keyboard toggle still works after gamepad close"), EventFor(Mappings.Toggle) == ETriggerEvent::Triggered);
		});
	});
}

#endif
