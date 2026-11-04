Additional Bubble VFX Variants

By Jessie Leung (Branch: 200-additional-bubble-vfx-variants)

# Purpose

The additional bubble variants can be used to fill environments with ambient bubbles or modified to be triggered through Blueprints. These systems are intended to remain lightweight and sprite-only, with both translucent and dithered masked material options.

Several common parameters for the emitters (color, size, lifetime, etc) as well as bubble materials (opacity multiplier, emission multiplier) are exposed for easy customization depending on the environment.

It can be referenced/reused as a bubble-like emitter by simply swapping out the sprite material.

# Assets

## Niagara Systems (ProjectAtlantis\\Effects\\Underwater\\AdditionalBubbles)

1. NS_CartoonBubble_Translucent
2. NS_CartoonBubble_Dithered
3. NS_CartoonBubble_Spark

## Materials (MaterialLibrary\\Effects\\Underwater\\AdditionalBubbles)

1. M_CartoonBubble_Translucent
2. M_CartoonBubble_ Dithered

These systems all produce a non-stop stream of bubble sprites that float upwards and remove themselves at the end of their lifetimes.

# Implementation

## NS_CartoonBubble_Translucent AND NS_CartoonBubble_Dithered

- These are functionally the same except for the sprite material. Perhaps dithered materials can reduce translucency costs compared to translucent ones, but this is a small effect that should not significantly affect performance

#### Exposed parameters and defaults

- Color (white) = Color of the emitter sprites
- Lifetime Min (5) = Minimum particle lifetime in seconds
- Lifetime Max (10) = Maximum particle lifetime in seconds
- Spawn rate (3) = Amount of particles spawned per second
- Uniform Curve Scale (1) = Scales overall size of particles
- Wobbleness (0.2) = Frequency of the squash and stretch of sprites. Set it to a non-whole number to avoid synced wobbles

### NS_CartoonBubble_Spark

- Mostly the same as its family, but lacks wobbliness for visual clarity, and disappears with a spark-shrink effect

#### Exposed parameters and defaults

- Same as NS_CartoonBubble_Translucent AND NS_CartoonBubble_Dithered

### M_CartoonBubble_Translucent

- Translucent bubble material for use on 2D sprite
- Includes noise generated panner distortion (localized wobble)
- 'Output Depth and Velocity' is enabled to reduce TSR ghosting on moving sprites

#### Exposed parameters and defaults

- Emission Multiplier = Multiplies emission value
- Opacity Multiplier = Multiplies opacity value

### M_CartoonBubble_Dithered

- Dithered masked bubble material for use on 2D sprite. Uses the bubble texture as a mask along with a radial mask centered on the sprite to control the dithered region
- Includes noise generated panner distortion (localized wobble)
- Proof of concept of using dithered materials instead of translucent ones

#### Exposed parameters and defaults

- Emission Multiplier = Multiplies emission value
- Opacity Multiplier = Multiplies opacity value
- Radius (0.38) = Dithered radius from center
- Randomness (0.8) = Randomness in applied dithering. Less means a more uniform banding

## Material Instances

- The Niagara systems use Material Instances of the bubble materials

# Usage

- The parameters should allow the emitters to be used from local bubble streams around corals to background effects
- Different colored bubbles with different wobble frequencies can suit various environment themes (such as: Fast red wobble bubble = Hot dangerous area, Slow green wobble bubble = Safe room)

# Showcase

- The emitters are placed in the \[Additional Bubbles\] room of the \[Lvl_Showcase_Materials\] map
- From left to right: Dithered → Translucent variants ×3 → Spark