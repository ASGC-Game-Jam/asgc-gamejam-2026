// Copyright (c) 2026 clarasmith-sound
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"

#include "SoundStyleWidgetInterface.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class USoundStyleWidgetInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Worn by Sound Style widgets (Button, Slider...) that carry their own Sound Style tags.
 * The subsystem asks for them through this, so it doesn't need to know every widget type.
 */
class SOUNDSTYLES_API ISoundStyleWidgetInterface
{
	GENERATED_BODY()

public:
	/** This widget's own Sound Style tags. Empty = use the screen's scope. */
	virtual FGameplayTagContainer GetOwnSoundClasses() const = 0;
};
