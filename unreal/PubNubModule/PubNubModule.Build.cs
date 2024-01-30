// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class PubNubModule : ModuleRules
{
    private bool OpenSsl = false;
    private bool StaticLink = false;

    public PubNubModule(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });
        PrivateDependencyModuleNames.AddRange(new string[] {  });


#if PLATFORM_WINDOWS
        string extention = StaticLink ? "lib" : "dll";
        string includeLib = OpenSsl ? "openssl" : "windows";
#elif PLATFORM_MAC
        string extention = StaticLink ? "a" : "dylib";
        string includeLib = OpenSsl ? "openssl" : "posix";
#else 
        string extention = StaticLink ? "a" : "so";
        string includeLib = OpenSsl ? "openssl" : "posix";
#endif

        if (OpenSsl) {
            PublicDependencyModuleNames.AddRange(new string[] { "OpenSSL" });
        }

        var path = Path.Combine(new string[] { ModuleDirectory, "..", ".." });
#if PLATFORM_WINDOWS
        PublicAdditionalLibraries.Add(Path.Combine(path, $"pubnub.{extention}"));
#else
        PublicAdditionalLibraries.Add(Path.Combine(path, $"libpubnub.{extention}"));
#endif
        PublicIncludePaths.AddRange(
            new string[] {
                Path.Combine(path, "core"),
                Path.Combine(path, "lib"),
                Path.Combine(path, includeLib)
            }
        );

        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));
    }
}
