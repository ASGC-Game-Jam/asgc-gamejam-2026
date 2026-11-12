// Copyright (c) 2026 clarasmith-sound
// SPDX-License-Identifier: MIT

#include "SoundStylesEditor.h"
#include "PropertyEditorModule.h"
#include "SoundStyleButton.h"
#include "SoundStyleButtonCustomization.h"


#define LOCTEXT_NAMESPACE "FSoundStylesEditorModule"

void FSoundStylesEditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	PropertyModule.RegisterCustomClassLayout(USoundStyleButton::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FSoundStyleButtonCustomization::MakeInstance));
}

void FSoundStylesEditorModule::ShutdownModule()
{
	// PropertyEditor may already be gone when the editor closes: only unregister if it's still loaded.
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomClassLayout(USoundStyleButton::StaticClass()->GetFName());
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSoundStylesEditorModule, SoundStylesEditor)