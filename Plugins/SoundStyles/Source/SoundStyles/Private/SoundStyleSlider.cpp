// Copyright (c) 2026 clarasmith-sound
// SPDX-License-Identifier: MIT

#include "SoundStyleSlider.h"
#include "Engine/GameInstance.h"
#include "SoundStyleTags.h"
#include "SoundStyleSubsystem.h"

TSharedRef<SWidget> USoundStyleSlider::RebuildWidget()
{
	TSharedRef<SWidget> Widget = Super::RebuildWidget();

	// Mouse and gamepad grab/let go do the same thing.
	OnMouseCaptureBegin.AddUniqueDynamic(this, &USoundStyleSlider::HandleCaptureBegin);
	OnMouseCaptureEnd.AddUniqueDynamic(this, &USoundStyleSlider::HandleCaptureEnd);
	OnControllerCaptureBegin.AddUniqueDynamic(this, &USoundStyleSlider::HandleCaptureBegin);
	OnControllerCaptureEnd.AddUniqueDynamic(this, &USoundStyleSlider::HandleCaptureEnd);
	OnValueChanged.AddUniqueDynamic(this, &USoundStyleSlider::HandleValueChanged);

	return Widget;
}

void USoundStyleSlider::ReleaseSlateResources(bool bReleaseChildren)
{
	USoundStyleSubsystem* Subsystem = GetSoundStyles();
	if (Subsystem)
	{
		Subsystem->StopUILoop(this);
	}

	Super::ReleaseSlateResources(bReleaseChildren);
}

void USoundStyleSlider::HandleCaptureBegin()
{
	bCaptured = true;
	LastStep = GetStepIndex();   // where the grab started: the first click comes when the handle leaves this slice

	USoundStyleSubsystem* Subsystem = GetSoundStyles();
	if (Subsystem)
	{
		Subsystem->PlayWidgetUISound(this, SoundStyles_Event_Press);
		Subsystem->StartUILoopWithParameters(this, SoundStyles_Event_Hold, MakeValueParameters());
	}
}

void USoundStyleSlider::HandleCaptureEnd()
{
	bCaptured = false;

	USoundStyleSubsystem* Subsystem = GetSoundStyles();
	if (Subsystem)
	{
		Subsystem->PlayWidgetUISound(this, SoundStyles_Event_Release);
		Subsystem->StopUILoop(this);
	}
}

USoundStyleSubsystem* USoundStyleSlider::GetSoundStyles() const
{
	return UGameInstance::GetSubsystem<USoundStyleSubsystem>(GetGameInstance());
}

void USoundStyleSlider::HandleValueChanged(float InValue)
{
	USoundStyleSubsystem* Subsystem = GetSoundStyles();
	if (!Subsystem)
	{
		return;
	}

	const TArray<FAudioParameter> Parameters = MakeValueParameters();

	// Not dragging = no loop = does nothing.
	Subsystem->SetUILoopParameters(this, Parameters);

	// Detents. Always remember the slice, but only click when the player is the one moving it.
	const int32 Step = GetStepIndex();
	const bool bPlayerMoving = bCaptured || HasAnyUserFocus();
	if (StepCount > 0 && bPlayerMoving && LastStep != INDEX_NONE && Step != LastStep)
	{
		Subsystem->PlayWidgetUISoundWithParameters(this, SoundStyles_Event_Step, Parameters);
	}
	LastStep = Step;
}

TArray<FAudioParameter> USoundStyleSlider::MakeValueParameters() const
{
	// Normalized: always 0-1, whatever the slider's Min/Max. Matches the MetaSound's Value input.
	TArray<FAudioParameter> Parameters;
	Parameters.Add(FAudioParameter(TEXT("Value"), GetNormalizedValue()));
	return Parameters;
}

int32 USoundStyleSlider::GetStepIndex() const
{
	return FMath::FloorToInt(GetNormalizedValue() * StepCount);
}
