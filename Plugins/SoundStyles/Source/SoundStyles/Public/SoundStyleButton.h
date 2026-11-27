// Copyright (c) 2026 clarasmith-sound
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "GameplayTagContainer.h"
#include "SoundStyleWidgetInterface.h"

#include "SoundStyleButton.generated.h"

class USoundStyleSubsystem;

/**
 * A Button that plays its UI sounds from the Sound Styles table: Hover (mouse or gamepad focus), Press, Release,
 * and the Hold loop while pressed.
 *
 * To convert an existing Button: select Sound Style Button in the Palette, then right-click the Button
 * in the Hierarchy > Replace With... > Replace With SoundStyleButton. Slot, style and bindings carry over.
 */
UCLASS(meta = (DisplayName = "Sound Style Button"))
class SOUNDSTYLES_API USoundStyleButton : public UButton, public ISoundStyleWidgetInterface
{
	GENERATED_BODY()

public:
	/** Sound style tags (UISound.Class...) for this button. Leave empty to use the screen's scope. Shown as "Sound Style" (not to be confused with Unreal Sound Classes). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound Style", meta = (DisplayName = "Sound Style", Categories = "UISound.Class"))
	FGameplayTagContainer SoundClasses;

	// ISoundStyleWidgetInterface
	virtual FGameplayTagContainer GetOwnSoundClasses() const override { return SoundClasses; }

#if WITH_EDITOR
	/** Groups all Sound Style widgets under one heading in the UMG Palette. */
	virtual const FText GetPaletteCategory() override { return NSLOCTEXT("SoundStyles", "PaletteCategory", "Sound Styles"); }
#endif

protected:
	/** Builds the Slate button and binds our sound handlers (clears any Button Style sounds first, with a warning). */
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** Clears Button Style sounds saved in the asset, quietly, so the validation warning doesn't repeat on every load. */
	virtual void PostLoad() override;

	/** The button's on-screen widget is being torn down (screen closed, removed...): stop its hold loop, since Release won't come. */
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

#if WITH_EDITOR
	/** A Button Style sound was set in the Details panel (e.g. a pasted style): clear it, with a warning. */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	/** Mouse moved over the button. */
	UFUNCTION()
	void HandleHover();

	/** Pressed: Press one-shot, then start the Hold loop. */
	UFUNCTION()
	void HandlePressed();

	/** Released: Release one-shot, then stop the Hold loop. */
	UFUNCTION()
	void HandleReleased();

	/** Gamepad/keyboard focus arrived: plays Hover, unless the mouse is already over it or a screen just opened. */
	void HandleFocus();

	/** Plays one event for this button through the subsystem. */
	void PlayEvent(FGameplayTag EventTag);

	/** Empties the Button Style's Hovered/Pressed/Clicked Sound slots. Returns true if any had a sound. */
	bool ClearStyleSounds();

	/** The Sound Styles subsystem for this button's game instance, or nullptr (e.g. in the designer). */
	USoundStyleSubsystem* GetSoundStyles() const;
};
