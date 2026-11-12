// Copyright (c) 2026 clarasmith-sound
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SoundStyleSettings.generated.h"

class USoundStyleTable;

/**
 * Project-wide settings for Sound Styles: which style table to use.
 */
UCLASS(config = Game, defaultconfig, meta=(DisplayName = "Sound Styles"))
class SOUNDSTYLES_API USoundStyleSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return FName(TEXT("Game")); }

	/** The style table that decides every UI sound in the game.*/
	UPROPERTY(config, EditAnywhere, Category= "Sound Styles")
	TSoftObjectPtr<USoundStyleTable> StyleTable;
};
