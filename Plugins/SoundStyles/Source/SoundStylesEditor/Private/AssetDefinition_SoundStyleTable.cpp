// Copyright (c) 2026 clarasmith-sound
// SPDX-License-Identifier: MIT

#include "AssetDefinition_SoundStyleTable.h"
#include "SoundStyleTableEditor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AssetDefinition_SoundStyleTable)

EAssetCommandResult UAssetDefinition_SoundStyleTable::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
	for (USoundStyleTable* Table : OpenArgs.LoadObjects<USoundStyleTable>())
	{
		TSharedRef<FSoundStyleTableEditor> Editor = MakeShared<FSoundStyleTableEditor>();
		Editor->InitEditor(OpenArgs.GetToolkitMode(), OpenArgs.ToolkitHost, Table);
	}
	return EAssetCommandResult::Handled;
}

