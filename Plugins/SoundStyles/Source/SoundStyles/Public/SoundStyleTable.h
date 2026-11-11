// Copyright (c) 2026 clarasmith-sound
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "AudioParameter.h"

#include "SoundStyleTable.generated.h"

class USoundBase;
class USoundControlBusMix;

/**
 * What one event sounds like for one class.
 * To limit fast repeats (e.g. sweeping across a list), set Sound Concurrency (Retrigger Time / Max Count) on the sound.
 */
USTRUCT(BlueprintType)
struct SOUNDSTYLES_API FSoundStyleEvent
{
	GENERATED_BODY()

	/** Sound to play. Empty means silent: stops this event inheriting from the parent class. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound Style")
	TObjectPtr<USoundBase> Sound;

	/**
	 * Optional. Fixed parameters sent to the sound when it starts, e.g. Brightness = 0.8 for this class.
	 * Game values sent at play time (With Parameters / Set UI Loop Parameters) override these by name.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound Style")
	TArray<FAudioParameter> Parameters;
};

/** One row of the stylesheet: the events one class sets. Events it doesn't list inherit from its parent tag. */
USTRUCT(BlueprintType)
struct SOUNDSTYLES_API FSoundStyleClass
{
	GENERATED_BODY()

	/** Event (Hover, Press...) -> its sound. Missing events inherit; listed with no sound = silent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound Style", meta = (Categories = "UISound.Event"))
	TMap<FGameplayTag, FSoundStyleEvent> Events;

	/** Optional. Active while a screen of this class is open (Push/Pop UI Audio State), e.g. duck the world under the pause menu. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound Style")
	TObjectPtr<USoundControlBusMix> WhileOpenMix;
};

/** The stylesheet: maps UI sound classes to the sounds they play. Owned by audio. */
UCLASS(BlueprintType)
class SOUNDSTYLES_API USoundStyleTable : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Class tag -> its row. The row for UISound.Class itself is the game-wide default. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound Style", meta = (Categories = "UISound.Class"))
	TMap<FGameplayTag, FSoundStyleClass> Classes;

	/**
	 * Finds which row answers for an event. The full lookup: levels in order, then the root row.
	 * A silent event (row lists the event, Sound is empty) still counts as an answer and stops the search.
	 * Used by the subsystem at runtime and by the table editor.
	 *
	 * @param Levels     Classes to check, in priority order. [0] = widget's own classes, [1] = nearest scope, ...
	 * @param EventTag   What happened, e.g. UISound.Event.Press.
	 * @param OutRowTag  Set to the class tag of the row that answered (UISound.Class if the root answered). Left empty if nothing answered.
	 * @return The event settings from the answering row, or nullptr if no row sets this event.
	 */
	const FSoundStyleEvent* Resolve(const TArray<FGameplayTagContainer>& Levels, FGameplayTag EventTag, FGameplayTag& OutRowTag) const;

private:
	/**
	 * Checks ONE class, then its parent, then its parent... stopping BEFORE the UISound.Class root.
	 * Returns the first row in that chain that lists the event.
	 * Example: Menu.Back -> Menu -> (stop)
	 *
	 * @param ClassTag   The class to start from.
	 * @param EventTag   The event to look for.
	 * @param OutRowTag  Set to the class tag of the row that answered.
	 * @return The event settings, or nullptr if nothing in the chain lists the event.
	 */
	const FSoundStyleEvent* FindInClassChain(FGameplayTag ClassTag, FGameplayTag EventTag, FGameplayTag& OutRowTag) const;

	/**
	 * Checks EVERY class in one level (calls FindInClassChain for each) and keeps the DEEPEST answering row.
	 * Classes outside UISound.Class are skipped with a warning.
	 * Two different rows at the same depth: the first one listed wins. With SoundStyles.Debug 1, warns once per row pair.
	 *
	 * @param LevelClasses  All classes at this level, e.g. a widget's own classes.
	 *                      (Not called "Classes": that would hide the table's own Classes map.)
	 * @param EventTag      The event to look for.
	 * @param OutRowTag     Set to the class tag of the winning row.
	 * @return The event settings from the deepest answering row, or nullptr if no class answered.
	 */
	const FSoundStyleEvent* FindInClasses(const FGameplayTagContainer& LevelClasses, FGameplayTag EventTag, FGameplayTag& OutRowTag) const;

	/**
	 * Row pairs already warned about as ambiguous, so the warning shows once per pair instead of on every hover.
	 * Mutable: lookups are const, but remembering a warning doesn't change what the table answers. Not saved.
	 */
	mutable TSet<TPair<FGameplayTag, FGameplayTag>> WarnedAmbiguousRowPairs;
};