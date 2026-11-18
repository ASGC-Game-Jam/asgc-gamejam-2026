// Copyright (c) 2026 clarasmith-sound
// SPDX-License-Identifier: MIT

#include "SoundStyleProgressBar.h"
#include "Engine/GameInstance.h"
#include "AudioParameter.h"
#include "SoundStyleTags.h"
#include "SoundStyleSubsystem.h"

void USoundStyleProgressBar::SetPercentWithSound(float InPercent)
{
	// Read the old value BEFORE writing the new one: that's all the detents need.
	const float OldPercent = FMath::Clamp(GetPercent(), 0.f, 1.f);
	const float NewPercent = FMath::Clamp(InPercent, 0.f, 1.f);
	SetPercent(InPercent);

	USoundStyleSubsystem* Subsystem = GetSoundStyles();
	if (!Subsystem || NewPercent == OldPercent)
	{
		return;   // nothing moved: no sound, and don't rewind the egg timer
	}

	TArray<FAudioParameter> Parameters;
	Parameters.Add(FAudioParameter(TEXT("Value"), NewPercent));

	// "Rewind the egg timer": just note the time. HandleQuiet compares against it, so nothing is re-registered per change.
	LastChangeTime = FPlatformTime::Seconds();

	// Fill loop: start on the first change (with one quiet check per frame), then just update it.
	if (!bFilling)
	{
		Subsystem->StartUILoopWithParameters(this, SoundStyles_Event_Fill, Parameters);
		bFilling = true;

		// Core ticker, not the world's timers: those stop while the game is paused, and this bar may live in a pause menu.
		QuietTimer = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &USoundStyleProgressBar::HandleQuiet));
	}
	else
	{
		Subsystem->SetUILoopParameters(this, Parameters);
	}

	// Detents: we know both the old and new slice, so no memory is needed.
	const int32 OldStep = FMath::FloorToInt(OldPercent * StepCount);
	const int32 NewStep = FMath::FloorToInt(NewPercent * StepCount);
	if (StepCount > 0 && NewStep != OldStep)
	{
		Subsystem->PlayWidgetUISoundWithParameters(this, SoundStyles_Event_Step, Parameters);
	}
}

bool USoundStyleProgressBar::HandleQuiet(float DeltaTime)
{
	// Real time, so it works the same paused or not.
	const bool bStillMoving = FPlatformTime::Seconds() - LastChangeTime < QuietSeconds;
	if (bStillMoving)
	{
		return true;   // check again next frame
	}

	StopFilling();
	return false;   // quiet: stop, and don't call again
}

void USoundStyleProgressBar::StopFilling()
{
	FTSTicker::RemoveTicker(QuietTimer);   // safe with an empty ticket, and from inside HandleQuiet
	QuietTimer.Reset();

	if (bFilling)
	{
		bFilling = false;

		USoundStyleSubsystem* Subsystem = GetSoundStyles();
		if (Subsystem)
		{
			Subsystem->StopUILoop(this);
		}
	}
}

void USoundStyleProgressBar::ReleaseSlateResources(bool bReleaseChildren)
{
	StopFilling();   // never leave a countdown pointing at a widget that's going away

	Super::ReleaseSlateResources(bReleaseChildren);
}

USoundStyleSubsystem* USoundStyleProgressBar::GetSoundStyles() const
{
	return UGameInstance::GetSubsystem<USoundStyleSubsystem>(GetGameInstance());
}
