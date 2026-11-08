# District 1 ocean and oxygen gardens

Open `/Game/ProjectAtlantis/Maps/POC/L_WaterSimulation_POC` and press Play.
This replaces the earlier river-surface experiment in the dedicated copy of
`L_Atlantis`. The original level and startup maps are unchanged.

## Explore

You control the real `BP_AtlantisPlayerCharacter`, using
`BP_AtlantisGameMode`, the existing swimming animation, input, ballast,
oxygen component, and HUD. You start submerged, facing the first air bubble.

- WASD and mouse: normal Atlantis movement; swimming follows camera pitch.
- Ballast 2 / 3 / 4: Descend / Wander / Ascend (shown on the existing HUD).
- Swim into a bubble and settle onto its raised floor to walk normally.
- Oxygen stops draining in a bubble and refills at 20 units per second.
- Leave the bubble to return to swimming and oxygen consumption.

| Location | Center / floor height (cm) |
| --- | --- |
| Player start | `(20200, -1200, 4400)` |
| Ocean surface | Z `106500` (roughly 1 km above the playable seabed) |
| Ocean floor | Z `3000` |
| Air pocket 1 | `(22000, 0, 3900)` / floor Z `3500` |
| Air pocket 2 | `(26500, 1500, 4700)` / floor Z `4300` |

Both pockets have a visible bubble shell, solid 13 m square walking floor,
warm local light, and an instruction sign. The spheres have no blocking collision:
swim through their boundaries. Route markers connect the two pockets.
The decorative shell meshes explicitly use the saved `NoCollision` profile.
Changing only Collision Enabled while leaving the `BlockAll` profile caused
the shells to become solid again when the map reloaded; that setup is fixed.

## How it works

Epic Water provides the `WaterBodyOcean`, water mesh, waves, and water-zone
rendering. Water Advanced remains enabled, but the shallow-water river
solver is not used for ocean traversal. Water rendering by itself does not
define the game's breathable spaces.

`AAtlantisWaterVolume` uses a box collision boundary with water enabled and
priority 10. Native character movement selects it below the ocean surface.
`AAtlantisAirPocketVolume` uses a spherical boundary, water disabled, and
priority 100. Native physics-volume selection takes the character out of
water inside the bubble. The existing movement component switches controls
and traversal, and the existing player state stops oxygen consumption.
The pocket restores oxygen only on authority and only to characters whose
active physics volume is that pocket. Touching the boundary from outside
does not grant oxygen.

Select `POC_SwimmingOceanVolume` to resize the swimming boundary. Its top
must match the ocean's surface. Select either `POC_AirPocket_*` volume to
adjust its sphere radius, priority, or oxygen restore rate. Resize the
matching visual bubble and walking floor to match gameplay geometry.
Keep dry-volume priority above water-volume priority.

The District 1 visual pass follows the reference boards on pages 8-10 of
FDS-004 District Design: pale stone foundations, gilded classical columns,
Art Nouveau curves, shell-like patinated roofs, jewel-coloured panels,
coral gardens, and warm oxygen-rest pavilions against teal water.
Two oxygen-garden pavilions surround the existing breathable pockets, with
open arches along the swimming route and a palace silhouette in the distance.
This is a reference-inspired POC with simple prototype geometry and palette
materials, not finished District 1 environment art or a complete district.

The deep-ocean look uses teal exponential distance fog, dim ambient fill,
fixed exposure, and warm local lights. It avoids a full-screen blue tint.
The sky and daylight surface are not visible from the play area.
The old blue scene-color tint and the copied tutorial's depth-based
`MPPI_Underwater_1` blendable are removed from this POC. That tutorial
volume re-enables itself during swimming, so merely disabling it in the
editor did not remove the blue effect. Shared source materials are unchanged.
The ocean's separate underwater post-process material is also disabled.

The main setup script calls `apply_deep_ocean_look.py`, followed by
`apply_district1_look.py`. The latter creates the reference palette and
`D1POC_*` decorative architecture/gardens, and applies the final District 1
lighting and fog. Use `POC_DeepFog`, `POC_DeepExposure`, and the local lights
to tune visibility. Decorative geometry uses NoCollision; the original
pocket floors and water/air physics volumes retain their gameplay roles.
The final `restore_underwater_readability.py` pass adds a dedicated
underwater-only depth-scattering material, slightly attenuates warm colour
over distance, adds subtle animated refraction and rising Niagara bubbles,
and softens the ambient fill. Unlike the old constant blue tint, visibility
depends on the amount of water between the camera and visible surfaces.
The effect is driven by the existing traversal-aware underwater volume and
switches off when the character enters breathable air. The entry regression
also checks this visual-state switch on entry and exit.
The haze material uses viewport-aware SceneTexture lookups for both colour
and depth. This fixes a rectangular haze cutoff in embedded PIE, where the
legacy SceneDepth UV fallback sampled a different render-buffer region.
Checked in the embedded viewport and after expanding it with F11; both
pockets also passed the movement-input entry/exit regression afterward.
The District 1 materials and fog were checked in PIE after saving and
reloading. Movement-input entry/exit validation still passes for both
oxygen gardens with the added architecture in place.

## Scope and validation

This is a single-player gameplay POC, with static air pockets. It does not
simulate compressed air, bubble deformation, changing ocean level, or
volumetric fluid displacement. The visible bubble shell is a VFX material;
the spherical physics volume defines the breathable region. Multiplayer
network behavior and packaged builds are not validated.

Validation: Development Editor / Win64 builds successfully. The focused
`Atlantis.Traversal.AirPocket` automation spec passes. In PIE, both pockets
were verified to switch the actual Atlantis pawn to Walking, stop oxygen
consumption, and restore oxygen; exiting restored Swimming and consumption.
Entering a pocket from ballast surface walking was also checked.

`Tools/WaterSimulation/validate_ocean_pie.py` repeats the five wet/dry
transition checks during PIE, writes `Saved/OceanPOC_Validation.json`, and
returns the pawn to its starting transform. It changes only the PIE session.
`Tools/WaterSimulation/validate_ocean_entry.py` checks physical access: it
positions the pawn outside each pocket, then uses movement input to swim
through the boundary, land, refill oxygen, and walk back into the ocean.
Boundary crossings do not use teleports. Results are written to
`Saved/OceanPOC_EntryValidation.json`.
The final saved-map validation passed all five stages. Map Check reported
zero errors and 176 warnings: 174 inherited Nanite/shadow-setting warnings,
one inherited empty mesh actor, and an empty-mesh warning on the generated
WaterBodyOcean actor. Ocean rendering and gameplay were verified in PIE.

The setup script is `Tools/WaterSimulation/build_ocean_playground.py`.
Run it only in the dedicated POC map; it removes the old POC actors and
recreates the ocean fixtures. Do not run it in `L_Atlantis`.

The editor is launched with `-SCCProvider=None` to avoid the Git plugin's
automatic-lock stall during external actor saves. Shell LFS uses the
configured Gitea endpoint, with the POC map lock retained.
