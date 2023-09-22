// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

// select desired module type

// `posix`, `openssl`, `windows`
static string Option = "posix";

// `posix`, `windows`
static string Architecture = "posix";

// `sync`, `callback`
static string Implementation = "sync";


public class PubNubModule : ModuleRules
{
    public PubNubModule(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });
        PrivateDependencyModuleNames.AddRange(new string[] {  });

        if (Option == "openssl") {
            PublicDependencyModuleNames.AddRange(new string[] { "OpenSSL" });
        }

        var path = Path.Combine(new string[] { ModuleDirectory, "..", ".." });
        var extention = Architecture == "posix" ? "a" : "lib";

        PublicAdditionalLibraries.Add(Path.Combine(path, Option, $"pubnub_{Implementation}.{extention}"));
        PrivateIncludePaths.AddRange(
            new string[] {
                path,
                Path.Combine(path, "core"),
                Path.Combine(path, "lib"),
                Path.Combine(path, Option)
            }
        );

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
