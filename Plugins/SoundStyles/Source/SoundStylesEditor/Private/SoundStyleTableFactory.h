// Copyright (c) 2026 clarasmith-sound
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"

#include "SoundStyleTableFactory.generated.h"

/**
 * Content Browser > right-click > Audio > Sound Styles > Sound Style Table.
 * New tables start with the UISound.Class root row (the game-wide default), ready for sounds.
 */
UCLASS()
class USoundStyleTableFactory : public UFactory
{
	GENERATED_BODY()

public:
	USoundStyleTableFactory();

	/** Makes the new table and adds the root row. */
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

	/** The Audio section of the "new asset" menu. */
	virtual uint32 GetMenuCategories() const override;

	/** The "Sound Styles" sub-menu inside Audio. */
	virtual const TArray<FText>& GetMenuCategorySubMenus() const override;

	/** "Sound Style Table". */
	virtual FText GetDisplayName() const override;
};
