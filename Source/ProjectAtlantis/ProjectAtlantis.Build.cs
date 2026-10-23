// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Linq;

public class ProjectAtlantis : ModuleRules
{
	public ProjectAtlantis(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"EnhancedInput", 
			"UMG", 
			"CommonInput", 
			"EngineSettings", 
			"AIModule", 
			"GameplayStateTreeModule",
			"GameplayTags",
			"StructUtils"
			});

        PrivateDependencyModuleNames.AddRange(new string[] {
            "GameAnalytics",
            "MetasoundFrontend",
            "MetasoundGraphCore",
            "MetasoundEngine",
            "MetasoundStandardNodes",
            "AudioExtensions",
            "HarmonixMetasound",
            "Harmonix"
        });
		PrivateDependencyModuleNames.AddRange(new string[] { "Analytics", "GameAnalytics", "CommonUI" });

		if (Target.ProjectDefinitions.Contains("WITH_SCREENSHOT_CAPTURE=1"))
		{
			DynamicallyLoadedModuleNames.Add("ScreenshotCapture");
		}

        PrivateIncludePathModuleNames.AddRange(new string[] { "GameAnalytics" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
