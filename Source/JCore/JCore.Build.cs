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
                "AIModule",
                "Core",
                "Engine",
                "UMG"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "AIModule",
                "CoreUObject",
                "CoreOnline",
                "Engine",
                "Slate",
                "SlateCore"
            }
        );
    }
}