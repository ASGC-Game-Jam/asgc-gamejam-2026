# 🎵 MetaSound Interface Template Guide

Welcome to the **Project Atlantis** Audio Interface System! This directory contains explicit boilerplate code templates used to register custom **Audio Parameter Interfaces** with Unreal Engine's MetaSound graph registry.

---

## Why is this code written as a "Template"? (Architectural Context)

If you have a strong software engineering background, your first instinct when looking at these files will likely be: *"This looks like boilerplate—why aren't we wrapping this in an abstract class or dynamic polymorphism?"*

**The short answer:** Unreal Engine's architectural layout won't allow it here.

1. **Static Engine Registry:** Unreal's `IAudioParameterInterfaceRegistry` registers pins at engine startup using static memory footprints. It depends entirely on preprocessor macros (like `AUDIO_PARAMETER_INTERFACE_MEMBER_DEFINE`) which execute before compilation. Because of this, they do not respect virtual tables or standard C++ class inheritance.
2. **Async Audio Thread Boundaries:** MetaSound graphs execute on an optimized, isolated high-priority audio thread. Passing polymorphic runtime classes over this thread boundary would risk memory race conditions or voice dropouts.
3. **Educational Purpose:** This project values explicit systems learning! By manually creating your data fields, namespaces, and sorting properties, you will directly learn how data contracts bind Unreal gameplay attributes to custom sound graphs.

---

## Step-by-Step: Creating a New Interface

To build a new gameplay audio interface contract (e.g., a `WeaponInterface` or `VehicleInterface`), use this template:

### 1. Duplicate the Template Files
Copy `AtlantisAudioInterface_PlayerCharacter.h` and `.cpp`, paste them into this folder, and rename them to your new system (e.g., `AtlantisAudioInterface_Weapon.h/.cpp`).

### 2. Update the `!!CHANGE THIS!!` Tags
Open your new files and locate the markers. You must rename:
* **The Namespace:** Change `PlayerCharacterInterface` to your system name (e.g., `WeaponInterface`).
* **The Unique Root Path:** Change `AUDIO_PARAMETER_INTERFACE_NAMESPACE` (e.g., from `"Atlantis.Character"` to `"Atlantis.Weapon"`). This string is how Unreal identifies the pin paths under the hood.
* **The Variables & Arrays:** Add your explicit `FLazyName` pins to the `Inputs` or `Outputs` namespaces, then populate an matching `.Add(...)` entry list inside the `FInterface()` constructor block.

### 3. Register It on Game Boot
Open your global module class layout (usually `AtlantisInterfaceRegistration.cpp`) and call your initialization routine during module boot:

```cpp
void FAtlantisAudioModule::StartupModule()
{
    // Existing registrations...
    ProjectAtlantisAudio::RegisterWeaponInterface();
}
```

---

## How to Use Your Interface in Gameplay Code

Once compiled, you can add your new interface via the **Interfaces** panel inside any MetaSound graph asset in the Unreal Editor. To feed gameplay variables into your sound safely without fragile string names, track them directly via your static compiled variables:

```cpp
#include "Audio/MetaSound/Interfaces/AtlantisAudioInterface_PlayerCharacter.h"
#include "Components/AudioComponent.h"

void AAtlantisCharacter::UpdateAudioTracking()
{
    if (UAudioComponent* AudioComp = GetAudioComponent())
    {
        // Safe, optimized, and string-less data updates!
        AudioComp->SetFloatParameter(ProjectAtlantisAudio::PlayerCharacterInterface::Inputs::CurrentHp, CurrentHealth);
        AudioComp->SetBoolParameter(ProjectAtlantisAudio::PlayerCharacterInterface::Inputs::IsHiding, bIsCharacterHiding);
    }
}
```

---

## Supported Data Types Mapping
When adding inputs to your array, ensure the C++ parameter type correctly maps to what MetaSound expects:
* `float` MetaSound Float
* `int32` MetaSound Integer *(Note: Do not use standard C++ `int`; always specify `int32`)*
* `bool` MetaSound Boolean
* `FName` / `FString` MetaSound String
