// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ProjectSP : ModuleRules
{
	public ProjectSP(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			
			// Input
			"EnhancedInput",
			
			// GAS
			"GameplayTags",
			"GameplayTasks",
			"GameplayAbilities",

			// AI
			"AIModule",

			// Modular Actors
			"ModularGameplay",
			"ModularGamePlayActors",
			
			// Common
			"CommonUI",
			
			// UMG
			"UMG"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"NavigationSystem"
		});
	}
}
