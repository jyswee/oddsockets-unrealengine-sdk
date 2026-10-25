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

        // Runtime module: no editor-only dependencies. Editor modules
        // (UnrealEd, Slate, ...) don't exist in cooked device builds, so
        // listing them here breaks iOS/Android packaging.
        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
            }
        );

        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
                // ... add any modules that your module loads dynamically here ...
            }
        );

        // Exceptions don't need RTTI; forcing bUseRTTI on fails the link
        // against the no-RTTI engine libs (undefined typeinfo for UObject etc).
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
