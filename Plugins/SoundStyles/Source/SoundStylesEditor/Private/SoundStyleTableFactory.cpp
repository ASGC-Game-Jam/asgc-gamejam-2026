// Copyright (c) 2026 clarasmith-sound
// SPDX-License-Identifier: MIT

#include "SoundStyleTableFactory.h"

#include "AssetTypeCategories.h"
#include "SoundStyleTable.h"
#include "SoundStyleTags.h"

#define LOCTEXT_NAMESPACE "SoundStyleTableFactory"

USoundStyleTableFactory::USoundStyleTableFactory()
{
	SupportedClass = USoundStyleTable::StaticClass();
	bCreateNew = true;      // show in the "new asset" menu
	bEditAfterNew = true;   // open the table editor right away
}

UObject* USoundStyleTableFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	USoundStyleTable* Table = NewObject<USoundStyleTable>(InParent, InClass, InName, Flags | RF_Transactional);

	// Start with the root row: every widget falls back to it, so it's the first thing to fill in.
	Table->Classes.Add(FGameplayTag(SoundStyles_Class), FSoundStyleClass());

	return Table;
}

uint32 USoundStyleTableFactory::GetMenuCategories() const
{
	return EAssetTypeCategories::Sounds;   // the "Audio" section of the right-click menu
}

const TArray<FText>& USoundStyleTableFactory::GetMenuCategorySubMenus() const
{
	static const TArray<FText> SubMenus = { LOCTEXT("SubMenu", "Sound Styles") };
	return SubMenus;
}

FText USoundStyleTableFactory::GetDisplayName() const
{
	return LOCTEXT("DisplayName", "Sound Style Table");
}

#undef LOCTEXT_NAMESPACE
