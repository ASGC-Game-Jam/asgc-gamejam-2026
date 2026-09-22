# Examine Mode

This document covers the implementation of the camera-switching prototype that will be used for the Investigation feature.

The implementation lives in `BP_AtlantisPlayerController`, which already contained a `FollowCamera`. I renamed `FollowCamera` to `ThirdPersonCamera` to distinguish it from the newly added `FirstPersonCamera`.

# Implementation

I used **[Feature] FDS-005 Investigation** to guide the implementation and evaluate different approaches to detecting objects during exploration.

According to the Investigation pod documentation, the player can freely enter Investigation mode while exploring. Based on this requirement, I experimented with several methods of detecting nearby or targeted objects.

# Iteration 1 — Jump Input

The first iteration used the **Jump** input as a simple way to test the camera switching.

I used a `FlipFlop` node to alternate between the two cameras, with `Set Active` nodes enabling and disabling the appropriate camera on each input.

This was primarily a prototype to verify that the camera switching itself worked correctly.

# Iteration 2 — Line Trace

The second iteration used a `Line Trace` from the camera's forward vector.

The trace returns the first object hit by the line, which can then be used to determine whether the player should switch from third-person to first-person.

This approach is useful for targeted interactions where the player needs to be looking directly at an object.

# Iteration 3 — Sphere Overlap

The third iteration used the `Sphere Overlap Actors` node to detect objects within a defined radius around the player.

The overlap returns an array of Actors. To retrieve the first Actor detected, I used a `Get` node with an index of `0`.

`Sphere Overlap` Actors also requires an `Object Types` array. I promoted this input to a variable and set the object type to `World Dynamic`.

# Collision Configuration

For the overlap detection to work, **Generate Overlap Events** must be enabled.

Under **Collision Responses**:

- Set **Visibility** to `Overlap`.
- Set **Camera** to `Overlap`.
- Set **World Static** to `Block`.
- Set **World Dynamic** to `Overlap`.

Leave the remaining object types as `Ignore`.
| Unity | Unreal |
|-------|--------|
| `Physics.OverlapSphere()` | `Sphere Overlap Actors` |
| `Physics.Raycast()` | `Line Trace By Channel` |