// Copyright (c) 2026 clarasmith-sound
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Components/CheckBox.h"
#include "GameplayTagContainer.h"
#include "SoundStyleWidgetInterface.h"

#include "SoundStyleCheckBox.generated.h"

class USoundStyleSubsystem;

/**
 * A CheckBox that plays its UI sounds from the Sound Styles table.
 * Player click: the Press event, sent with Checked (true = now on). Code changes (Set Is Checked) stay silent.
 */
UCLASS(meta = (DisplayName = "Sound Style CheckBox"))
class SOUNDSTYLES_API USoundStyleCheckBox : public UCheckBox, public ISoundStyleWidgetInterface
{
	GENERATED_BODY()

public:
	/** Sound style tags (UISound.Class...) for this checkbox. Leave empty to use the screen's scope. Shown as "Sound Style" (not to be confused with Unreal Sound Classes). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound Style", meta = (DisplayName = "Sound Style", Categories = "UISound.Class"))
	FGameplayTagContainer SoundClasses;

	// ISoundStyleWidgetInterface
	virtual FGameplayTagContainer GetOwnSoundClasses() const override { return SoundClasses; }

#if WITH_EDITOR
	/** Groups all Sound Style widgets under one heading in the UMG Palette. */
	virtual const FText GetPaletteCategory() override { return NSLOCTEXT("SoundStyles", "PaletteCategory", "Sound Styles"); }
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	/** The player toggled it. */
	UFUNCTION()
	void HandleCheckStateChanged(bool bIsChecked);

	/** The Sound Styles subsystem for this checkbox's game instance, or nullptr (e.g. in the designer). */
	USoundStyleSubsystem* GetSoundStyles() const;
};
