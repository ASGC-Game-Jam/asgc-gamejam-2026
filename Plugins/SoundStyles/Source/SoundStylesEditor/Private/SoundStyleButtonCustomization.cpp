// Copyright (c) 2026 clarasmith-sound
// SPDX-License-Identifier: MIT

#include "SoundStyleButtonCustomization.h"
#include "Components/Button.h"

#include "DetailLayoutBuilder.h"

#define LOCTEXT_NAMESPACE "FSoundStyleButtonCustomization"

TSharedRef<IDetailCustomization> FSoundStyleButtonCustomization::MakeInstance()
{
	return MakeShareable(new FSoundStyleButtonCustomization);
}

void FSoundStyleButtonCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	DetailBuilder.HideProperty(TEXT("WidgetStyle.HoveredSlateSound"), UButton::StaticClass());
	DetailBuilder.HideProperty(TEXT("WidgetStyle.PressedSlateSound"), UButton::StaticClass());
	DetailBuilder.HideProperty(TEXT("WidgetStyle.ClickedSlateSound"), UButton::StaticClass());
}

#undef LOCTEXT_NAMESPACE
