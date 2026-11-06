# MetaSound Interface Template Guide

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
`AtlantisAudioInterface_MusicAxes.h/.cpp` is the template: it keeps the `!!CHANGE THIS!!` markers. Copy the header into `Public/Audio/MetaSound/Interfaces/` and the `.cpp` into `Private/Audio/MetaSound/Interfaces/`, then rename them to your new system (e.g., `AtlantisAudioInterface_Weapon.h/.cpp`).

`AtlantisAudioInterface_Environment.h/.cpp` and `AtlantisAudioInterface_MusicLayers.h/.cpp` are examples of finished copies, with the markers removed. Environment uses integer pins. MusicLayers uses float pins with default values, so a graph previews correctly in the MetaSound editor before the game sets them.

### 2. Update the `!!CHANGE THIS!!` Tags
Open your new files and locate the markers. You must rename:
* **The Namespace:** Change `MusicAxesInterface` to your system name (e.g., `WeaponInterface`).
* **The Unique Root Path:** Change `AUDIO_PARAMETER_INTERFACE_NAMESPACE` (e.g., from `"Atlantis.Music.Axes"` to `"Atlantis.Weapon"`). This string is how Unreal identifies the pin paths under the hood.
* **The Register Function:** Change `RegisterMusicAxesInterface` to match (e.g., `RegisterWeaponInterface`).
* **The Variables & Arrays:** Add your explicit `FLazyName` pins to the `Inputs` namespace in **both** files with the **same names** (the header's `extern` is a promise the `.cpp` must keep, or you get a linker error when gameplay code uses the pin), then populate a matching `.Add(...)` entry inside the `FInterface()` constructor block with the right data type (see the table at the bottom).

Then clean up:
* **Delete what you don't use.** If your interface has no outputs, remove every `Outputs` block entirely; don't leave them commented out.
* **Delete the `!!CHANGE THIS!!` markers and the template banner** once you're done, so finished code doesn't look unfinished. Only the MusicAxes template keeps them.

> [!WARNING]
> **No dots in pin names.** In audio parameter names `.` is the namespace separator, and Unreal splits at the **last** dot. `AUDIO_PARAMETER_INTERFACE_MEMBER_DEFINE("Room.Type")` becomes `Atlantis.Environment.Room.Type`, which shows up as a pin called `Type` in a namespace called `Atlantis.Environment.Room`. Use `RoomType` instead.

### 3. Register It on Game Boot
Open `Private/Audio/MetaSound/Interfaces/AtlantisAudioInterfaceRegistration.cpp`, include your header, and call your register function inside `RegisterAllMetaSoundInterfaces()` (the game module calls this once at startup):

```cpp
#include "Audio/MetaSound/Interfaces/AtlantisAudioInterface_Weapon.h"

void RegisterAllMetaSoundInterfaces()
{
    RegisterMusicAxesInterface();
    RegisterEnvironmentInterface();
    RegisterMusicLayersInterface();
    RegisterWeaponInterface();
}
```

If your interface doesn't appear in a MetaSound's **Interfaces** panel after building, this step was missed.

---

## How to Use Your Interface in Gameplay Code

Once compiled, you can add your new interface via the **Interfaces** panel inside any MetaSound graph asset in the Unreal Editor. To feed gameplay variables into your sound safely without fragile string names, track them directly via your static compiled variables:

```cpp
#include "Audio/MetaSound/Interfaces/AtlantisAudioInterface_MusicAxes.h"
#include "Components/AudioComponent.h"

void UpdateMusicAxes(UAudioComponent* AudioComp, float PlayerState, float Tension)
{
    if (AudioComp)
    {
        // Uses the compiled pin names, so a typo is a build error rather than a silent miss.
        AudioComp->SetFloatParameter(ProjectAtlantisAudio::MusicAxesInterface::Inputs::PlayerStateAxis, PlayerState);
        AudioComp->SetFloatParameter(ProjectAtlantisAudio::MusicAxesInterface::Inputs::TensionAxis, Tension);
    }
}
```

Most gameplay code shouldn't set pins directly, though. Gameplay sends facts through `SetAudioDataPayload` (the audio front door), and the audio subsystem decides how they reach these pins. Intermittent facts like `Atlantis.Environment`'s `RoomType` are delivered in batches by parameter pack (#105).

---

## Tuning the Music

All music tuning lives in one asset: **`DA_AtlantisMusicAxisMapping`**, set in **Project Settings > Game > Atlantis Audio Settings**. Designers shouldn't need to edit code or graph maths to tune the music.

| Section | What it controls | Reaches the music through |
|---|---|---|
| **Music Axes** | Where Player State and Tension rest (the baselines), and how each gameplay fact pushes them (the rules) | `Atlantis.Music.Axes` |
| **Music Layers** | The Player State value where Ambient switches to Active (`ActiveLayerSplit`), and the Tension value where Passive comes in (`PassiveLayerSplit`) | `Atlantis.Music.Layers` |

In `MSS_AtlantisAmbientMusic`, each layer is chosen with `Axis - Split + 1`, then `To Int` and `Clamp 0..1`. This gives 1 at or above the split and 0 below it. The comment boxes in the graph say the same.

**Live tuning:** in the editor, changes to the asset during Play are heard straight away. The audio subsystem listens for edits to the mapping asset and resends the axes and split points. This works from the Details panel or from an editor utility widget.

> [!WARNING]
> **Tuning during Play edits the real asset**, not a temporary copy, which is the opposite of how Play normally behaves. Changes stay in the asset after you stop. Save them if you want to keep them, or revert the asset if you don't.

---

## Supported Data Types Mapping
When adding inputs to your array, ensure the C++ parameter type correctly maps to what MetaSound expects:
* `float` → MetaSound Float
* `int32` → MetaSound Int32 *(Note: Do not use standard C++ `int`; always specify `int32`)*
* `bool` → MetaSound Bool
* `FString` → MetaSound String
