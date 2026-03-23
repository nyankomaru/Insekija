// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Inseki : ModuleRules
{
	public Inseki(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"Inseki",
			"Inseki/Variant_Platforming",
			"Inseki/Variant_Platforming/Animation",
			"Inseki/Variant_Combat",
			"Inseki/Variant_Combat/AI",
			"Inseki/Variant_Combat/Animation",
			"Inseki/Variant_Combat/Gameplay",
			"Inseki/Variant_Combat/Interfaces",
			"Inseki/Variant_Combat/UI",
			"Inseki/Variant_SideScrolling",
			"Inseki/Variant_SideScrolling/AI",
			"Inseki/Variant_SideScrolling/Gameplay",
			"Inseki/Variant_SideScrolling/Interfaces",
			"Inseki/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
