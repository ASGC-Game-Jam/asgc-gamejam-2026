// Copyright (c) 2026 clarasmith-sound
// SPDX-License-Identifier: MIT

#include "SoundStyleButton.h"
#include "Engine/GameInstance.h"
#include "SoundStyleTags.h"
#include "SoundStyleSubsystem.h"

TSharedRef<SWidget> USoundStyleButton::RebuildWidget()
{
	if(ClearStyleSounds())
	{
		UE_LOG(LogSoundStyles, Warning, TEXT("Sound Style Button '%s': cleared the Button Style's Hovered/Pressed/Clicked Sound. UI sounds come from the Sound Styles table."), *this->GetName());
	}

	TSharedRef<SWidget> Widget = Super::RebuildWidget();

	// Unique: RebuildWidget can run more than once. Blueprint bindings on the same events keep working.
	// OnReceivedFocus is a single-cast native delegate, so this is its only binding.
	OnHovered.AddUniqueDynamic(this, &USoundStyleButton::HandleHover);
	OnPressed.AddUniqueDynamic(this, &USoundStyleButton::HandlePressed);
	OnReleased.AddUniqueDynamic(this, &USoundStyleButton::HandleReleased);
	OnReceivedFocus.BindUObject(this, &USoundStyleButton::HandleFocus);

	return Widget;
}

void USoundStyleButton::PostLoad()
{
	Super::PostLoad();

	ClearStyleSounds();
}

void USoundStyleButton::ReleaseSlateResources(bool bReleaseChildren)
{
	USoundStyleSubsystem* Subsystem = GetSoundStyles();
	if (Subsystem)
	{
		Subsystem->StopUILoop(this);
	}

	Super::ReleaseSlateResources(bReleaseChildren);
}

#if WITH_EDITOR
void USoundStyleButton::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (ClearStyleSounds())
	{
		UE_LOG(LogSoundStyles, Warning, TEXT("Sound Style Button '%s': cleared the Button Style's Hovered/Pressed/Clicked Sound. UI sounds come from the Sound Styles table."), *this->GetName());
	}
}
#endif

void USoundStyleButton::PlayEvent(FGameplayTag EventTag)
{
	USoundStyleSubsystem* Subsystem = GetSoundStyles();
	if (Subsystem)
	{
		Subsystem->PlayWidgetUISound(this, EventTag);
	}
}

void USoundStyleButton::HandleHover()
{
	PlayEvent(SoundStyles_Event_Hover);
}

void USoundStyleButton::HandlePressed()
{
	PlayEvent(SoundStyles_Event_Press);
	USoundStyleSubsystem* Subsystem = GetSoundStyles();
	if (Subsystem)
	{
		Subsystem->StartUILoop(this, SoundStyles_Event_Hold);
	}
}

void USoundStyleButton::HandleReleased()
{
	PlayEvent(SoundStyles_Event_Release);

	USoundStyleSubsystem* Subsystem = GetSoundStyles();
	if (Subsystem)
	{
		Subsystem->StopUILoop(this);
	}
}

bool USoundStyleButton::ClearStyleSounds()
{
	FButtonStyle Style = GetStyle();
	if(!Style.HoveredSlateSound.GetResourceObject() && !Style.PressedSlateSound.GetResourceObject() && !Style.ClickedSlateSound.GetResourceObject())
	{
		return false;
	}

	const FSlateSound EmptySound;
	Style.HoveredSlateSound = EmptySound;
	Style.PressedSlateSound = EmptySound;
	Style.ClickedSlateSound = EmptySound;

	SetStyle(Style);

	return true;
}

void USoundStyleButton::HandleFocus()
{
	// A mouse click also focuses the button: the hover already played.
	if (IsHovered())
	{
		return;
	}

	// A screen that just opened is setting focus from code, not the player.
	USoundStyleSubsystem* Subsystem = GetSoundStyles();
	if (Subsystem && Subsystem->IsInOpenSettleWindow())
	{
		return;
	}

	PlayEvent(SoundStyles_Event_Hover);
}

USoundStyleSubsystem* USoundStyleButton::GetSoundStyles() const
{
	return UGameInstance::GetSubsystem<USoundStyleSubsystem>(GetGameInstance());
}