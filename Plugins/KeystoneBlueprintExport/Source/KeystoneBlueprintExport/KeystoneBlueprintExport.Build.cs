// Keystone Blueprint Export — editor module build rules. Editor-only (depends on UnrealEd
// and BlueprintGraph), so it never links into a packaged game.
using UnrealBuildTool;

public class KeystoneBlueprintExport : ModuleRules
{
    public KeystoneBlueprintExport(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "CoreUObject",
            "Engine",
            "Projects",          // IPluginManager, project paths
            "Json",              // TJsonWriter
            "UnrealEd",          // editor-only: UBlueprint access, save delegates, commandlet host
            "BlueprintGraph",    // UK2Node, UEdGraph pins
            "AssetRegistry",     // enumerate all Blueprints
            "ToolMenus",         // the Keystone menu
            "Slate",
            "SlateCore",
        });
    }
}
