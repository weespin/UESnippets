using UnrealBuildTool;

public class SplineHelper : ModuleRules
{
    public SplineHelper(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "UnrealEd",
                "Slate",
                "SlateCore",
                "EditorStyle",
                "PropertyEditor",
                "InputCore",
                "DetailCustomizations"
            });

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "ComponentVisualizers",
                "DetailCustomizations"
            });

        PrivateIncludePaths.Add("Editor/DetailCustomizations/Private");
    }
}