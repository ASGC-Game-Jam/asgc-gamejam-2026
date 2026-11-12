// Copyright (c) 2026 clarasmith-sound
// SPDX-License-Identifier: MIT

#pragma once

#include "SoundStyleTable.h"
#include "AssetDefinitionDefault.h"

#include "AssetDefinition_SoundStyleTable.generated.h"

/**
 * How the Content Browser treats Sound Style Table assets: name, colour, category (Audio > Sound Styles),
 * and double-click opens the table editor (FSoundStyleTableEditor).
 */
UCLASS()
class UAssetDefinition_SoundStyleTable : public UAssetDefinitionDefault
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_SoundStyleTable", "Sound Style Table"); }

	virtual FLinearColor GetAssetColor() const override { return FLinearColor(FColor(220,191,255)); }

	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return USoundStyleTable::StaticClass(); }

	/** Opens one table editor per selected table. */
	virtual EAssetCommandResult OpenAssets(const FAssetOpenArgs& OpenArgs) const override;

	/** Audio > Sound Styles, in the Content Browser filters. (The "new asset" menu comes from USoundStyleTableFactory.) */
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto Categories = { EAssetCategoryPaths::Audio / NSLOCTEXT("SoundStyles", "SoundStylesSubMenu", "Sound Styles") };
		return Categories;
	}
	// UAssetDefinition End
};
