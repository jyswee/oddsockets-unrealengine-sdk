using UnrealBuildTool;

public class OddSockets : ModuleRules
{
    public OddSockets(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
                // ... add public include paths required here ...
            }
        );

        PrivateIncludePaths.AddRange(
            new string[] {
                // ... add other private include paths required here ...
            }
        );

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "HTTP",
                "WebSockets",
                "Json",
                "JsonUtilities"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Slate",
                "SlateCore",
                "ToolMenus",
                "EditorStyle",
                "EditorWidgets",
                "UnrealEd",
                "BlueprintGraph",
                "KismetCompiler",
                "PropertyEditor",
                "Sockets",
                "Networking"
            }
        );

        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
                // ... add any modules that your module loads dynamically here ...
            }
        );

        // Enable RTTI for proper exception handling
        bUseRTTI = true;
        
        // Enable exceptions for error handling
        bEnableExceptions = true;

        // Definitions for the module
        PublicDefinitions.Add("ODDSOCKETS_API=DLLEXPORT");
        PublicDefinitions.Add("WITH_ODDSOCKETS=1");

        // Platform-specific settings
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicDefinitions.Add("PLATFORM_WINDOWS=1");
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            PublicDefinitions.Add("PLATFORM_MAC=1");
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            PublicDefinitions.Add("PLATFORM_LINUX=1");
        }
        else if (Target.Platform == UnrealTargetPlatform.IOS)
        {
            PublicDefinitions.Add("PLATFORM_IOS=1");
        }
        else if (Target.Platform == UnrealTargetPlatform.Android)
        {
            PublicDefinitions.Add("PLATFORM_ANDROID=1");
        }

        // Optimization settings
        OptimizeCode = CodeOptimization.InShippingBuildsOnly;
    }
}
