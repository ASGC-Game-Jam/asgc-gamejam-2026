// Copyright (c) 2026 clarasmith-sound
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"

#include "SoundStyleScopeInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class USoundStyleScopeInterface : public UInterface
{
	GENERATED_BODY()
};

class SOUNDSTYLES_API ISoundStyleScopeInterface
{
	GENERATED_BODY()

public:
	/**
	 * Sound classes this widget's children inherit, e.g. UISound.Class.Menu for a menu screen.
	 * Buttons inside this widget with no classes of their own use these.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Sound Styles")
	FGameplayTagContainer GetUISoundClasses() const;
};