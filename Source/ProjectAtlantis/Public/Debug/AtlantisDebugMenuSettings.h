// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "InputCoreTypes.h"
#include "AtlantisDebugMenuSettings.generated.h"

class UInputAction;
class UInputMappingContext;

/** Runtime Enhanced Input objects built from the configured debug keys. */
struct FAtlantisDebugInputMappings
{
	UInputMappingContext* Gameplay = nullptr;
	UInputMappingContext* Menu = nullptr;
	UInputAction* Toggle = nullptr;
	UInputAction* GamepadToggle = nullptr;
	UInputAction* Reset = nullptr;
	UInputAction* Close = nullptr;
};

/** Development menu bindings, following Screenshot Capture's Game.ini settings pattern. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Debug Menu"))
class PROJECTATLANTIS_API UAtlantisDebugMenuSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UAtlantisDebugMenuSettings();

	/** None disables this shortcut. Restart the game or PIE after changing bindings. */
	UPROPERTY(Config, EditAnywhere, Category = "Input")
	FKey ToggleMenuKey = EKeys::F7;

	UPROPERTY(Config, EditAnywhere, Category = "Input")
	FKey ToggleMenuGamepadKey = EKeys::Gamepad_Special_Right;

	/** Hold this button while pressing the gamepad toggle. None allows a single button. */
	UPROPERTY(Config, EditAnywhere, Category = "Input")
	FKey ToggleMenuGamepadModifier = EKeys::Gamepad_LeftThumbstick;

	UPROPERTY(Config, EditAnywhere, Category = "Input", meta = (DisplayName = "Restart Game Key"))
	FKey ResetPlayerKey = EKeys::F10;

	/** Restarts the game only while the debug menu is open, preserving gameplay bindings. */
	UPROPERTY(Config, EditAnywhere, Category = "Input", meta = (DisplayName = "Restart Game Gamepad Key"))
	FKey ResetPlayerGamepadKey = EKeys::Gamepad_RightThumbstick;

	FAtlantisDebugInputMappings CreateInputMappings(UObject* Outer) const;
};
