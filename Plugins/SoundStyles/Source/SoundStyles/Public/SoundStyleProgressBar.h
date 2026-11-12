// Copyright (c) 2026 clarasmith-sound
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Components/ProgressBar.h"
#include "Containers/Ticker.h"
#include "GameplayTagContainer.h"
#include "SoundStyleWidgetInterface.h"

#include "SoundStyleProgressBar.generated.h"

class USoundStyleSubsystem;

/**
 * A Progress Bar that sounds as it moves. Call Set Percent With Sound instead of Set Percent:
 * the Fill loop plays while it keeps moving (stops after Quiet Seconds without a change),
 * and a Step one-shot plays each time it crosses a detent. Both get Value (0-1).
 */
UCLASS(meta = (DisplayName = "Sound Style Progress Bar"))
class SOUNDSTYLES_API USoundStyleProgressBar : public UProgressBar, public ISoundStyleWidgetInterface
{
	GENERATED_BODY()

public:
	/** Sound style tags (UISound.Class...) for this bar. Leave empty to use the screen's scope. Shown as "Sound Style" (not to be confused with Unreal Sound Classes). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound Style", meta = (DisplayName = "Sound Style", Categories = "UISound.Class"))
	FGameplayTagContainer SoundClasses;

	/** Detents: plays the Step event each time the bar moves into a new slice. 10 = a click every 10%. 0 = no Step sounds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound Style", meta = (ClampMin = "0", UIMax = "50"))
	int32 StepCount = 10;

	/** How long the bar must stay still before the Fill loop stops. Longer than the gap between your updates. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound Style", meta = (ClampMin = "0.05", Units = "s"))
	float QuietSeconds = 0.2f;

	// ISoundStyleWidgetInterface
	virtual FGameplayTagContainer GetOwnSoundClasses() const override { return SoundClasses; }

#if WITH_EDITOR
	/** Groups all Sound Style widgets under one heading in the UMG Palette. */
	virtual const FText GetPaletteCategory() override { return NSLOCTEXT("SoundStyles", "PaletteCategory", "Sound Styles"); }
#endif

	/** Sets the percent (0-1) and plays the bar's sounds. Plain Set Percent stays silent. */
	UFUNCTION(BlueprintCallable, Category = "Sound Style")
	void SetPercentWithSound(float InPercent);

protected:
	/** Torn down while filling (screen closed): cancel the countdown and stop the loop. */
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
	/** Runs every frame while filling. Stops the loop once nothing has changed for QuietSeconds. Returns true = keep checking. */
	bool HandleQuiet(float DeltaTime);

	/** Cancels the countdown and stops the Fill loop (if playing). */
	void StopFilling();

	/** The Sound Styles subsystem for this bar's game instance, or nullptr (e.g. in the designer). */
	USoundStyleSubsystem* GetSoundStyles() const;

	/** Ticket for the quiet check (runs while filling). Empty when not filling. */
	FTSTicker::FDelegateHandle QuietTimer;

	/** Real time (FPlatformTime::Seconds) of the last change that made a sound. */
	double LastChangeTime = 0.0;

	/** True while the Fill loop has been started and not stopped. */
	bool bFilling = false;
};
