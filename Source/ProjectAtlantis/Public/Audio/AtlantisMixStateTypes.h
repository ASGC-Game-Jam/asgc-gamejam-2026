// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "AtlantisMixStateTypes.generated.h"

class USoundControlBusMix;

/** One mix state: Mix is on while WhileFact is true. */
USTRUCT(BlueprintType)
struct FAtlantisMixStateRule
{
	GENERATED_BODY()

	/** The bool fact that turns the mix on while it's true. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mix", meta = (Categories = "Audio.Fact"))
	FGameplayTag WhileFact;

	/** The control bus mix to turn on. Its attack and release times set the fade. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mix")
	TSoftObjectPtr<USoundControlBusMix> Mix;
};
