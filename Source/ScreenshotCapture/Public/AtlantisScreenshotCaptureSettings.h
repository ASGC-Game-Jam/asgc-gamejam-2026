// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "InputCoreTypes.h"
#include "AtlantisScreenshotCaptureSettings.generated.h"

/** Configures the screenshot capture shortcut independently of gameplay input mappings. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Screenshot Capture"))
class SCREENSHOTCAPTURE_API UAtlantisScreenshotCaptureSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UAtlantisScreenshotCaptureSettings();

	/** Keyboard key or gamepad button used to capture. None disables the shortcut. */
	UPROPERTY(Config, EditAnywhere, Category = "Input")
	FKey CaptureKey = EKeys::F9;
};
