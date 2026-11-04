// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class ProjectAtlantisTarget : TargetRules
{
	public ProjectAtlantisTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;

		// Keep the opt-out in target rules so UBT tracks it when invalidating cached build graphs.
		bool bWithScreenshotCapture = !(Target.Arguments?.HasOption("-WithoutScreenshotCapture") ?? false);
		ProjectDefinitions.Add("WITH_SCREENSHOT_CAPTURE=" + (bWithScreenshotCapture ? "1" : "0"));

		ExtraModuleNames.AddRange( new string[] { "ProjectAtlantis" } );
	}
}
