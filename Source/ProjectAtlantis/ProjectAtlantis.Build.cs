// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Linq;

public class ProjectAtlantis : ModuleRules
{
	public ProjectAtlantis(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.Add("DeveloperSettings");

		PublicDependencyModuleNames.AddRange(new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"EnhancedInput",
				"UMG",
				"CommonInput",
				"EngineSettings",
				"AIModule",
				"StateTreeModule",
				"GameplayStateTreeModule", 
				"GameplayDebugger",
        			"GameplayTags",
			  "StructUtils",

                "Inkpot",
                "InkpotEditor"
			});

		PrivateDependencyModuleNames.AddRange(new string[]
			{
				"GameAnalytics",
				"MetasoundFrontend",
				"MetasoundGraphCore",
				"MetasoundEngine",
				"MetasoundStandardNodes",
				"AudioExtensions",
				"HarmonixMetasound",
				"Harmonix",
				"SmartObjectsModule",
				"Analytics",
				"CommonUI",
				"AudioGameplay",
				"AudioGameplayVolume",
				// Swimming AI builds FNavigationPath for the engine's path following.
				"NavigationSystem",
				// State Tree property references (TStateTreePropertyRef) resolve through this module.
				"PropertyBindingUtils"
			});

		if (Target.ProjectDefinitions.Contains("WITH_SCREENSHOT_CAPTURE=1"))
		{
			DynamicallyLoadedModuleNames.Add("ScreenshotCapture");
		}

		PrivateIncludePathModuleNames.AddRange(new string[] { "GameAnalytics" });

		// Raw SVG fallbacks are read by Slate at runtime, outside the cooked asset system.
		if (Target.Configuration != UnrealTargetConfiguration.Shipping)
		{
				RuntimeDependencies.Add("$(ProjectDir)/Content/ProjectAtlantis/UI/Debug/Icons/*.svg", StagedFileType.NonUFS);
		}

		// Uncomment if you are using Slate UI
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
