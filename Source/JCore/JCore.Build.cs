// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class JCore : ModuleRules
{
    public JCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "Engine",
                "UMG"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "CoreOnline",
                "Engine",
                "Slate",
                "SlateCore"
            }
        );
    }
}