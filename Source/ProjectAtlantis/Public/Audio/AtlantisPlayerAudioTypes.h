// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"

#include "AtlantisPlayerAudioTypes.generated.h"

class USoundBase;

/** How a player audio row plays its sound. */
UENUM(BlueprintType)
enum class EAtlantisPlayerAudioBehaviour : uint8
{
	/** Plays while its Play While Fact is true, and stops when it turns false. */
	Loop UMETA(DisplayName = "Loop"),
	/** Plays once each time its Play While Fact turns true. */
	OneShot UMETA(DisplayName = "One Shot"),
};

/** One sound the player audio subsystem plays, and the facts that control it. Use one row per sound. */
USTRUCT(BlueprintType)
struct FAtlantisPlayerAudioRow : public FTableRowBase
{
	GENERATED_BODY()

	/** The sound to play, usually a MetaSound. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundBase> Sound = nullptr;

	/** Whether the sound loops while its fact is true, or plays once when it turns true. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	EAtlantisPlayerAudioBehaviour Behaviour = EAtlantisPlayerAudioBehaviour::Loop;

	/** The bool fact that controls this sound, such as Audio.Fact.Player.Underwater. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound", meta = (Categories = "Audio.Fact"))
	FGameplayTag PlayWhileFact;

	/** Also requires the player to be underwater, such as for an alarm that shouldn't sound on land. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	bool bUnderwaterOnly = false;

	/** The fact whose value is sent to the MetaSound while it plays, such as Audio.Fact.Oxygen.Level. Loops only. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound", meta = (Categories = "Audio.Fact", EditCondition = "Behaviour == EAtlantisPlayerAudioBehaviour::Loop"))
	FGameplayTag ParameterFact;

	/** The MetaSound input that Parameter Fact's value is sent to, such as Oxygen. Must match the input's name exactly. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound", meta = (EditCondition = "Behaviour == EAtlantisPlayerAudioBehaviour::Loop"))
	FName ParameterName;
};