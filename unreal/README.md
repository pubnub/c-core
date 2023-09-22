# Unreal Engine integration

## Overview

Our SDK can be used together with [UnrealEngine](https://www.unrealengine.com/en-US). 
It is **not** fully functional plugin but can be easly integrated into your project as a unreal module.

## How to setup

First of all you have to clone this repository inside the `<UnrealProject>/Source/` directory. 
We recommend to place it inside the `ThirdParty` one (create it if no present). 

After that you have to compile desired option of the SDK. You can simple do it as follow:
```sh
make -C <option> -f <architecture>.mk pubnub_<implementation>.a 
```

for example:

```sh
make -C posix -f posix.mk pubnub_sync.a
```

After that you have to adjust `PubNubModule/PubNubModule.Build.cs` with selected options. 
Just change `option`, `architecture` and `implementation` with the same values you used in compilation. 

> Note that this step is temporary until we enchant our build system that is quite obsolete and need maintenance. We are aware of it.

for example:
```cs 
static string option = "posix";
static string architecture = "posix";
static string option = "sync";
```

In the end you have to import module in your your project as follow:

- `<UnrealProject>.Target.cs` and `<UnrealProject>Editor.Target.cs`
```cs
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

Now you can import `PubNub.h` header into your files as follow:
```cpp
#include "PubNub.h" 
```

> Note that you don't have to wrap it with `THIRD_PARTY_INCLUDES_START` and `THIRD_PARTY_INCLUDES_END` because we've done that for you.

Good luck with your project!

