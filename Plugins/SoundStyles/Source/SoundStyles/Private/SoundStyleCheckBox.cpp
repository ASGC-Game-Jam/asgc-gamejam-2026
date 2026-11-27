// Copyright (c) 2026 clarasmith-sound
// SPDX-License-Identifier: MIT

#include "SoundStyleCheckBox.h"
#include "Engine/GameInstance.h"
#include "AudioParameter.h"
#include "SoundStyleTags.h"
#include "SoundStyleSubsystem.h"

TSharedRef<SWidget> USoundStyleCheckBox::RebuildWidget()
{
	TSharedRef<SWidget> Widget = Super::RebuildWidget();

	// Fires only for player clicks: Set Is Checked from code doesn't broadcast it, so loading saved options stays silent.
	OnCheckStateChanged.AddUniqueDynamic(this, &USoundStyleCheckBox::HandleCheckStateChanged);

	return Widget;
}

void USoundStyleCheckBox::HandleCheckStateChanged(bool bIsChecked)
{
	USoundStyleSubsystem* Subsystem = GetSoundStyles();
	if (!Subsystem)
	{
		return;
	}

	// One event, with the new state as data: the MetaSound decides how on and off differ.
	TArray<FAudioParameter> Parameters;
	Parameters.Add(FAudioParameter(TEXT("Checked"), bIsChecked));
	Subsystem->PlayWidgetUISoundWithParameters(this, SoundStyles_Event_Press, Parameters);
}

USoundStyleSubsystem* USoundStyleCheckBox::GetSoundStyles() const
{
	return UGameInstance::GetSubsystem<USoundStyleSubsystem>(GetGameInstance());
}
