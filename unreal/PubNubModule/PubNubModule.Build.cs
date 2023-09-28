// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;



public class PubNubModule : ModuleRules
{
    // select desired module type
    
    // `posix`, `openssl`, `windows`
    private readonly string Option = "posix";
    
    // `posix`, `windows`
    private readonly string Architecture = "posix";
    
    // `sync`, `callback`
    private readonly string Implementation = "sync";

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
        PublicIncludePaths.AddRange(
            new string[] {
                path,
                Path.Combine(path, "core"),
                Path.Combine(path, "lib"),
                Path.Combine(path, Option)
            }
        );

        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
