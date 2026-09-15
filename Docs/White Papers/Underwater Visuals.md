Underwater Visuals

By 𝓐𝓝astasia 𝓚𝓘seleva and Michael Verret

# Custom Solution vs the UE5 Water Plugin

When coming up with an implementation for a new feature, one must always ask whether an existing solution already exists and satisfies all the requirements for that feature. If an existing solution does not exist or doesn't satisfy all the requirements, then a custom solution must be developed.

Unreal Engine 5 contains a Water Plugin that accomplishes many of the visual effects one would want and expect for underwater visuals. This is an out-of-the-box solution that you just need to enable. However, this plugin has some drawbacks. The biggest of these drawbacks, in my mind, is that it is still experimental. Being experimental means, it is not feature complete, not all the existing features are fully functional and can have bugs, and its continued existence remains in doubt. Any future version of Unreal Engine may remove this experimental plugin. Another drawback I see this plugin having is that it is a very complex implementation that needs fixes and tweaks to be fully functional. The complexity makes it difficult to maintain and future updates to the engine could change any part of it…potentially drastically. The complexity of this plugin and the look and feel of the visuals it creates are also not really in line with a cozy stylized game that is nearly always underwater, which Project Atlantis aims to be.

Given the state of the existing solution, a custom solution has been implemented. This custom solution is less complex than UE5's water plugin and is completely within our control. The reduced complexity makes it easier to understand and maintain. Being completely within our control means we can tweak any part of it to get the exact look and feel we desire for Project Atlantis. This custom solution is comprised of a post processing volume, a post processing material and some VFX and materials to help sell the look and feel of an underwater environment. Two variants, so far, have been created to demonstrate different techniques that can be used to accomplish these visuals. A showcase level has been created so that both of these variants, and additional VFX and materials, can be viewed and explored to help better determine the resource cost of these techniques and which will deliver the desired look and feel for Project Atlantis.

# Showcase Level/Map

The map, Lvl_Showcase_Materials, was created to demonstrate the work that has been accomplished in our efforts to produce a believable underwater environment that aligns with the desired look and feel of Project Atlantis. This map allows users/players to easily and efficiently experience each of the underwater visual variants as well as other VFX and materials that help portray an underwater environment. This will enable better feedback on where the visuals and implementation need to go and a better understanding of the resource costs of each technique.

To facilitate showcasing each of the elements involved, we created a simple Blueprint (BP_Showcase_PostProcess) containing a basic blockout and a Post Process Volume enclosed within a collision box. The BP can be placed directly into the level and customized through the Details panel, with a text field for naming each instance and an option to assign a custom material. The PP effect becomes active when entering the BPs volume, allowing different instances to have their own setups. This makes it possible to test and compare multiple configurations simultaneously within the same map, simply by walking between the different BP instances

# Variant 1

This variant achieves underwater visuals using a post processing material and water caustics (via a decal material). This variant has a single version of a bubble.

## Post Processing Material

This material includes the following effects:

- Fog
  - Creates the effect of water/fog by coloring the screen with a believable water color and applying a light scattering color to it
- Scene Depth Attenuation
  - Attenuates the fog color based on scene depth. The color is lighter for things that are closer and darker for things that are further away.
- Distortion
  - Creates a wavy distortion effect across the screen as if viewing the environment through water.

Several parameters have been added to allow for easier tweaking of these effects to better match the desired look and feel for Project Atlantis.

## Caustics

The water caustics for this variant are implemented with a decal material. The caustics mask is panned at four different speeds. The overlapping effect of this gives it a kind of wavy/wobbly look that is close to how water caustics actually look and behave. When applied to a volume, the effect is projected onto whatever surfaces in the environment are within that volume. Not being a light source, this effect remains relatively unaffected by environmental lighting and likely requires less tweaking.

## Bubbles

The bubble in this variant of the post processing effects utilizes a simple transparent material applied to a sphere mesh. A panner node fed into the World Position Offset (WPO) causes a wavy warping to the surface of the sphere and produces a more dynamic bubble effect.

# Variant 2

This variant achieves underwater visuals using a post processing material and water caustics (via a light function material). There are two different variations of bubbles for this variant, one using a sprite renderer and another using a mesh renderer. These are attributes of the Niagara System particle emitter.

## Post Processing Material

The post processing material for this variant is largely based on the tutorial videos from Ben Cloward. It includes the following effects:

- Screen Warping
  - Creates a wavy distortion effect across the screen as if viewing the environment through water. The effect is minimized at the edges of the screen using a square edge mask so that artifacts are not introduced at the edges of the screen where there aren't enough pixels to one side to create the effect
- Lens Distortion
  - Creates an effect where everything is a bit offset from their physical position as if viewing the environment through a lens/mask. The effect is bound by a center vignette mask like the shape of a lens/mask.
- Blur
  - The screen center and six points around the center are sampled to create a blurring effect to further enhance the effect of viewing the environment through the distortion of water. This is also bound by a center vignette mask.
- Color Attenuation (Fog)
  - The entire screen is tinted by a color value equivalent to water. The effect is altered based on the distance from the camera, lighter for things closer to the camera, darker for things that are further.

Several parameters have been added to allow for easier tweaking of these effects to better match the desired look and feel for Project Atlantis.

## Caustics

The water caustics for this variant are implemented with a light function material. A mask was created using Substance Designer. This mask is panned in two different directions with a noise texture applied to it that is also being panned. This gives it a kind of wavy/wobbly look that is close to how water caustics actually look and behave. When applied to a light source, the effect is projected onto whatever surfaces in the environment the light hits. A spotlight, like in the showcase level only projects the effect within the cone of the light. To have the effect projected in all directions, like in a cave type setting, you would need to use a point light. Being a light source the effect is more heavily influenced by other lighting in the environment and might need more tweaking than a decal based caustics effect.

## Bubbles

There are two variants of bubbles in this variant of the underwater visuals. They each have their own material and their own Niagara System particle emitter. One uses a sprite-based renderer and an image of light reflected off a sphere is applied to these 2D sprites. The other uses a mesh-based renderer, and a translucent PBR material is applied to a simple sphere mesh. The material also adds a fake light ring around the edges of the bubble using a Fresnel node so that the bubbles are more noticeable and stylized. Because it uses a true PBR material with refraction enabled, these bubbles will react properly to whatever environment they are in. Because of all these calculations and using an actual mesh, these bubbles are a lot more expensive than the sprite-based alternative.

# Oxygen

The material creates a fake liquid volume inside an existing 3D object by using the object's local coordinates and masks to define the container's boundaries and the minimum/maximum possible liquid levels. A controllable liquid-level value determines where the liquid surface sits within that volume, while procedural animation can make the surface move and ripple. The shader then uses these calculations to decide which parts of the object's surface should appear as liquid, creating the illusion of a separate liquid filling the real object without requiring actual liquid geometry or a fluid simulation. We also created a simple Niagara system to spawn bubbles (using a mesh renderer) and placed it inside the tank. This adds a basic sense of movement and life to the already dynamic liquid.

A future improvement to this effect would be to have a simple panning texture/image of bubbles vs the much more costly particle system. The small enclosed environment of the oxygen display is more conducive to a lower detail implementation without sacrificing visual quality and readability.

# Ground Material

A stylized sandy sea floor inspired material was created using Substance Designer and applied to the floor of the showcase level. There are three variants of this material. One does not include height information, one includes height information via a technique called "bump offset", and a third includes height information via a technique called "world position offset."

The first variant is obviously the most resource efficient but also produces the least believable effect as no matter what the material is it remains perfectly flat.

The second variant utilizes the "bump offset" technique, is the next most efficient and produces a decent height effect. The effect simply "bumps" pixels in a direction based on the camera vector and the height information of the material. This can lead to some distortion in the texture of the material (the starfish for example). This effect also diminishes or is completely negated when an object is resting on the surface as it does not actually cover things that should be in front. For example, the waves of sand should be rising above the plane of the surface they are applied to. Nothing is actually rising off of the flat floor surface though, so if a ball were placed in a location of the surface that should be behind one of these waves of sand…you would still see the parts of the ball that should be behind the wave…since there is no real height to the material.

The third variant uses the "world position offset" technique and is a little more expensive as it manipulates actual geometry. In the showcase level the material will still appear flat as the floor surface it is applied to does not have enough geometry for the effect to work. If the floor were subdivided then the vertices would be pushed higher or lower based on the height information of the material. This effect would not be diminished by objects resting on the surface as the actual geometry of the floor would be manipulated and would obscure things that are behind higher points in the material.

Additional variants that could be created would be POM (Parallax Occlusion Mapping) and nanite tessellation. These techniques are far more expensive but do produce much better height effects. For Project Atlantis they may be too resource intensive and may provide too much detail for the stylized look and feel that is desired.