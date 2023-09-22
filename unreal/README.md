# Unreal Engine integration

## Overview

Our SDK can be used together with [UnrealEngine](https://www.unrealengine.com/en-US). 
It is **not** fully functional plugin but can be easly integrated into your project as a unreal module.

## How to setup

First of all you have to clone this repository inside the `<UnrealProject>/Source/` directory. 
We recommend to place it inside the `ThirdParty` one (create it if no present). 

After that you have to compile [desired option](https://www.pubnub.com/docs/sdks/c-core#hello-world) of the SDK.
You can simple do it in the SDK directory as follow:
```sh
make -C <option> -f <architecture>.mk pubnub_<implementation>.a 
```

For example:

```sh
make -C openssl -f posix.mk pubnub_sync.a
```

> :warning: if you choose `openssl` option make sure that your openssl library headers match the Unreal ones! :warning:

After that you have to adjust `PubNubModule/PubNubModule.Build.cs` with selected options. 
Just change `option`, `architecture` and `implementation` with the same values you used in compilation. 

> Note that this step is temporary until we enchant our build system that is quite obsolete and need maintenance. We are aware of it.

for example:
```csharp 
static string Option = "openssl";
static string Architecture = "posix";
static string Implementation = "sync";
```

In the end you have to import module into your project as follow:

- `<UnrealProject>.Target.cs` and `<UnrealProject>Editor.Target.cs`
```csharp
public class <UnrealProject>[Editor]Target : TargetRules
{
  public <UnrealProject>[Editor]Target (TargetInfo Target) : base(Target)
  {
    //...
    ExtraModuleNames.Add("PubNubModule");
  }
}
```

- `<UnrealProject>.uproject`
```json5
{
  //...
  "Modules": [
    //...
    {
      "Name": "PubNubModule",
      "Type": "Runtime",
      "LoadingPhase": "Default"
    }
  ],
  //...
}
```

Now generate the project files using your IDE and you're ready to go!

## How to use it 

Make it discoverable in module that you want to use it by `<Module>.Build.cs` file:
```csharp
public class <Module> : ModuleRules
{
  public <Module>(ReadOnlyTargetRules Target) : base(Target)
  {
    //...
    PrivateDependencyModuleNames.Add("PubNubModule");
  }
}
```

Now you can import `PubNub.h` header into your files as follow:
```cpp
#include "PubNub.h" 
```

> Note that you don't have to wrap it with `THIRD_PARTY_INCLUDES_START` and `THIRD_PARTY_INCLUDES_END` because we've done that for you.


Good luck with your project!
