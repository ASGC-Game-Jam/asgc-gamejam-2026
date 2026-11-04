# Hiding Spots

## Overview

Hiding spots are smart objects with a single slot. The player can claim the slot in order to hide. 
Because hiding spots are smart objects, NPCs will be able to query them in the future, for example to search spots where the player might be hiding. This is not implemented yet.

## Architecture

`Core/Interactables/BP_HidingSpot_Base` is the base for hiding spot assets. It doesn't have a mesh component; it uses a collision sphere to intercept player's interactions by blocking the `Interact` trace channel; it implements `Interact` from `BPI_Interact`.

`SOD_HidingSpot` is the Smart Object definition linked to the Smart Object component of `BP_HidingSpot_Base`.

`UAtlantisHidingSpotBehaviourDef` is the class used to identify smart object slots as hiding spots. This is a data class and should contain the necessary properties to describe behaviours different from default. The class is currently empty since we have only one type of hiding spot. If we want to introduce a new type of hiding spot that behaves differently, this class will contain the properties needed to tell the two types apart and describe their behaviours.

`UAtlantisHidingComponent` contains the enter\exit logic and exposes `IsHiding()` and the `OnHidingStateChanged` event. `AAtlantisPlayerCharacter` has this component.

`Placeables/BP_HidingSpot` is a `BP_HidingSpot_Base` child and should be used to place hiding spots in the level.


## How it works

### Entering

When the player interacts with a hiding spot, `UAtlantisHidingComponent::EnterHidingSpot` (on the player):

1. Finds the hiding slot on the spot and claims it.
2. Moves the player to the slot's transform (scale reset to 1, so scaling a spot never scales the player) and hides the character in game.
3. Stops the character's movement: velocity is reset and the movement component is deactivated.
4. Restricts the input mapping contexts listed in `RestrictedMappingContexts`.
5. Turns the view to face the slot's direction. 
6. Broadcasts `OnHidingStateChanged(true)`.

The camera switches to first person while hiding. The camera boom's collision test is disabled in first person, so the camera stays in place inside tight spots.

### While hiding

By default the player can look around and interact (to exit), but cannot move. Restricted controls are set by the `RestrictedMappingContexts` on the HidingComponent in BP_AtlantisPlayerCharacter.

### Exiting

Interacting again calls `ExitHidingSpot`, which:

1. Removes the input restrictions and reactivates movement.
2. Moves the player back to where they were before entering, facing the direction they are currently looking.
3. Makes the character visible again.
4. Releases the slot and broadcasts `OnHidingStateChanged(false)`.

### Detection

The player is undetected while hiding, only if the hiding spot is actually behind cover.
Even if the character is hidden in game, it is still detectable from enemies. In order for the player to be undetected, visibility must be blocked by the assets surrounding the hiding spot, or the mesh added to the blueprint.

Open questions:

* Should enemies detect the player if they can still see them? For example if they look directly through the hiding spot entrance?
* Should enemies search hiding spot if they saw or heard the player near them? And if they didn't?


## Placing a hiding spot

1. Drag `BP_HidingSpot` into the level.
2. The base Blueprint has no mesh. Place it in an enclosure made of level assets (for example a gap between rocks), or add a mesh in a child Blueprint to create a reusable type of hiding spot. For this hiding spot, meshes with an aperture would work best, a set of rocks, a hollow tree. If the mesh is something like a crate or a barrel, with no apertures, make sure it's hollow. If it has something like a peephole, it may be a good idea to define a new SOD and differentiate the two in the code updating the logic and putting the requested property in the behaviour class.
3. Scale and position the interaction sphere so it is easy to reach around the entrance, without poking through surrounding walls: a sphere sticking out on the far side of a wall can be interacted with from there. If you need a different shape, set the sphere collision to ignore "Interact" trace channel and add another shape to block it and ignore everything else.
4. The arrow on the slot gizmo shows the direction the player faces when entering. If the hiding spot has an opening the player can look through, the arrow should point toward it.
5. The hiding spot shoud be placed behind cover, surrounded by level assets or covered by adding a mesh to the blueprint, otherwise enemies will be able to detect the player.


## Improvements and future work

- Restricted controls are the same for every hiding spot. If needed, the array can be moved to the behaviour class so each hiding spot can choose which controls to restrict.
- The camera snaps between third and first person; a fade would look better.
- Peeking is not implemented, the player can only move the camera and look through openings in the hiding spot.
- There is no visual clue for hiding spots at the moment. We may use the existing highlight or maybe some custom "darkness" material to make hiding spots entrance distinctive, such a material should be transparent or disabled from the inside.
- NPC searching of hiding spots is not implemented.
