// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "AtlantisAudioPayloadTypes.generated.h"

// Generic payload shapes for audio facts. The fact tag gives the data its meaning; each tag's struct is
// registered in Project Settings > Game > Atlantis Audio Settings > Atlantis Audio Validation.

/** A single number. The tag defines its meaning and range (e.g. Audio.Fact.Oxygen.Level is a 0-1 fraction). */
USTRUCT(BlueprintType)
struct PROJECTATLANTIS_API FAtlantisAudioFloatPayload
{
	GENERATED_BODY()

	/** The number being reported. See the fact tag's description for its meaning and range. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlantis Audio|Payloads")
	float Value = 0.f;
};

/** A single yes/no. The tag defines what true means (e.g. Audio.Fact.Stealth.Hiding is true while hiding). */
USTRUCT(BlueprintType)
struct PROJECTATLANTIS_API FAtlantisAudioBoolPayload
{
	GENERATED_BODY()

	/** The yes/no being reported. See the fact tag's description for what true means. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlantis Audio|Payloads")
	bool bValue = false;
};

/** A state belonging to one instance, e.g. one enemy or one investigation. Key says which one; State says what it's doing. */
USTRUCT(BlueprintType)
struct PROJECTATLANTIS_API FAtlantisAudioKeyedStatePayload
{
	GENERATED_BODY()

	/** Which instance this is about, e.g. an enemy's name or an investigation's title. Must stay the same for that instance across sends. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlantis Audio|Payloads")
	FName Key;

	/** The instance's current state. Pick an Audio.State tag that matches the fact, e.g. Audio.State.Enemy.* for Audio.Fact.Enemy.Status. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlantis Audio|Payloads", meta = (Categories = "Audio.State"))
	FGameplayTag State;
};
