// Copyright (c) 2026 clarasmith-sound
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Components/Slider.h"
#include "GameplayTagContainer.h"
#include "AudioParameter.h"
#include "SoundStyleWidgetInterface.h"

#include "SoundStyleSlider.generated.h"

class USoundStyleSubsystem;

/**
 * A Slider that plays its UI sounds from the Sound Styles table.
 * Grab: Press + the Hold loop. Let go: Release, loop stops. Works with mouse and gamepad.
 */
UCLASS(meta = (DisplayName = "Sound Style Slider"))
class SOUNDSTYLES_API USoundStyleSlider : public USlider, public ISoundStyleWidgetInterface
{
	GENERATED_BODY()

public:
	/** Sound style tags (UISound.Class...) for this slider. Leave empty to use the screen's scope. Shown as "Sound Style" (not to be confused with Unreal Sound Classes). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound Style", meta = (DisplayName = "Sound Style", Categories = "UISound.Class"))
	FGameplayTagContainer SoundClasses;

	// ISoundStyleWidgetInterface
	virtual FGameplayTagContainer GetOwnSoundClasses() const override { return SoundClasses; }

#if WITH_EDITOR
	/** Groups all Sound Style widgets under one heading in the UMG Palette. */
	virtual const FText GetPaletteCategory() override { return NSLOCTEXT("SoundStyles", "PaletteCategory", "Sound Styles"); }
#endif

	/** Detents: plays the Step event (with Value) each time the handle moves into a new slice. 10 = a click every 10%. 0 = no Step sounds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound Style", meta = (ClampMin = "0", UIMax = "50"))
	int32 StepCount = 10;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** Torn down mid-drag (screen closed): stop the loop, since the capture-end won't come. */
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
	/** Handle grabbed (mouse or gamepad). */
	UFUNCTION()
	void HandleCaptureBegin();

	/** Handle let go (mouse or gamepad). */
	UFUNCTION()
	void HandleCaptureEnd();


	/** The value moved (drag, click on the track, keys, or code): update the loop, and play a Step if the player crossed a detent. */
	UFUNCTION()
	void HandleValueChanged(float InValue);

	/** [Value = position 0-1], sent to the Hold loop and the Step one-shots. */
	TArray<FAudioParameter> MakeValueParameters() const;

	/** The Sound Styles subsystem for this slider's game instance, or nullptr (e.g. in the designer). */
	USoundStyleSubsystem* GetSoundStyles() const;

	/** Which slice the value is in now: 0 .. StepCount. */
	int32 GetStepIndex() const;

	/** True between grab and let go (mouse or gamepad). */
	bool bCaptured = false;

	/** The slice the value was in last time it changed. INDEX_NONE (-1) = not known yet. */
	int32 LastStep = INDEX_NONE;
};
