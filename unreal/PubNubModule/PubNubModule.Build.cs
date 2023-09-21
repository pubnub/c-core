// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class PubNubModule : ModuleRules
{
	public PubNubModule(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });
        PublicDependencyModuleNames.AddRange(new string[] { "OpenSSL" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		PublicAdditionalLibraries.Add(Path.Combine(new string[] { ModuleDirectory, "..", "..", "posix", "pubnub_sync.a" }));
        PrivateIncludePaths.Add(Path.Combine(new string[] { ModuleDirectory, "..", ".." }));
        PrivateIncludePaths.Add(Path.Combine(new string[] { ModuleDirectory, "..", "..", "core" }));
        PrivateIncludePaths.Add(Path.Combine(new string[] { ModuleDirectory, "..", "..", "lib" }));
        PrivateIncludePaths.Add(Path.Combine(new string[] { ModuleDirectory, "..", "..", "posix" }));

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
