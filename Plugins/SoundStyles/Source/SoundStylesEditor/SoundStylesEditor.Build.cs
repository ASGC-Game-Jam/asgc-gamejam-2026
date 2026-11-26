// Copyright (c) 2026 clarasmith-sound
// SPDX-License-Identifier: MIT

using UnrealBuildTool;

public class SoundStylesEditor : ModuleRules
{
	public SoundStylesEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
				
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
			);
			
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"UMG",
				"SoundStyles",
				"PropertyEditor",
				"AssetDefinition",
				"UnrealEd",
				"AudioModulation",   // While Open Mix picker (USoundControlBusMix)
				"GameplayTags",
				"GameplayTagsEditor",   // "+ Add Row" class tag picker (SGameplayTagCombo)
				"InputCore",
				"AudioExtensions",   // event Parameters panel (FAudioParameter)
				"AssetTools",        // factory menu category (EAssetTypeCategories)
			}
			);
	}
}
