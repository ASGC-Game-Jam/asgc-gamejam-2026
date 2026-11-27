// Copyright (c) 2026 clarasmith-sound
// SPDX-License-Identifier: MIT

using UnrealBuildTool;

public class SoundStyles : ModuleRules
{
	public SoundStyles(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
				
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"GameplayTags",
				"UMG",
				"DeveloperSettings",
				"AudioExtensions",
			}
			);
			
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
				"AudioModulation",
			}
			);
	}
}
