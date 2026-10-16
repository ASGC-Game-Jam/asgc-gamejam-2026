// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "InputCoreTypes.h"
#include "AtlantisDebugMenuSettings.generated.h"

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
	FKey ToggleMenuGamepadKey = EKeys::Gamepad_Special_Left;

	UPROPERTY(Config, EditAnywhere, Category = "Input")
	FKey ResetPlayerKey = EKeys::F10;

	UPROPERTY(Config, EditAnywhere, Category = "Input")
	FKey ResetPlayerGamepadKey = EKeys::Gamepad_RightThumbstick;

	bool IsToggleKey(const FKey& Key) const;
	bool IsResetKey(const FKey& Key) const;
};
