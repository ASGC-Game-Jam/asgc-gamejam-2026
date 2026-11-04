// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class ProjectAtlantisEditorTarget : TargetRules
{
	public ProjectAtlantisEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		bool bWithScreenshotCapture = !(Target.Arguments?.HasOption("-WithoutScreenshotCapture") ?? false);
		ProjectDefinitions.Add("WITH_SCREENSHOT_CAPTURE=" + (bWithScreenshotCapture ? "1" : "0"));

		ExtraModuleNames.AddRange( new string[] { "ProjectAtlantis" } );
	}
}
