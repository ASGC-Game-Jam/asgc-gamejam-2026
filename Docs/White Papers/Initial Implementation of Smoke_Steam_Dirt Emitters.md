# [Initial Implementation of Smoke/Steam/Dirt Emitters](https://github.com/ASGC-Game-Jam/asgc-gamejam-2026/issues/178)

## By Alan Lee

# The Goal of these Emitters

The use case of these niagara system emitters is to provide a set of flexible, modular lightweight emitters that can be used in early prototyping for environment levels for faster iteration cycles. 

This gaseous family of visual effects (smoke, steam, dirt clouds) all follow a similar particle set up and behavior, so just by swapping the output and using different 2D textures for billboards, or using a simple 3D mesh and shader. Many visual styles can be quickly created to explore different artstyle directions for environments. 

By exposing a small set of user parameters, designers and environmental artists are able to make quick adjustments to these emitters to block out or create environmental effects, such as various broken machinery smoke, player foot step dirt clouds, or high pressured steam emission and clouds. 

# Implementation

## General Implementation There is a total of 6 Niagara System (NS) emitters:

1. NS\_Dirt\_Burst  
2. NS\_DirtStylized\_Burst  
3. NS\_Smoke\_Continuous  
4. NS\_SmokeStylized\_Continuous  
5. NS\_ToonSteam\_Burst  
6. NS\_ToonSteam\_Continuous  
   

All 6 of these emitters follow a similar NS particle setup, the particle output all spawn using a random outward velocity following a spherical shape, against a drag. Then scale up in size quickly over time while dissolving or fading out towards the end. 

A custom scratchpad module is used to determine the velocity of each particle,  to allow for a much more flexible shape of the output. It allows the user to control both a random width spread and height spread, as well as a velocity minimum and maximum. 

For instance, by default, particles fire along the x axis, so by setting spread width to 360, it now fires in 360 degrees around the z axis, creating a ring. And by adjusting the height spread to 90 (both directions), it now fires in a full sphere. Different angle spread settings can create a thin cone, a semi circle, dome, etc.

This creates the basis of a simple, gaseous form and motion that can be further modified to create Steam/Smoke/Dirt. Small changes were made to the base implementation to create the 6 variations listed above.

## 3D Toon Steam

This version outputs a 3D mesh, and uses a simple cel-shaded material to create a toon look. The steam material erodes over time using a cloud texture mask to create a bubbly, foam looking dissolve effect. 

The cel-shader currently **does not** take the game’s real light direction into account, and the shadows direction is hardcoded with alignment to the showcase levels light direction for prototyping purposes.

Two versions of the toon Steam shader are available, a continuous and a burst version. The burst version emits all particles at once, while the continuous version emits them over time steadily.

## 2D Smoke & Dirt Clouds

This version outputs a 2D billboard particle and uses a soft 2x2 procedurally generated cloud/smoke texture to create a soft, blended smoke and dirt clouds look. 

The smoke emitters have a gravity field (+Z) in the particle update to simulate an upwards floating behavior. Smoke emitters also fire continuously and scale in size more steadily over time, before rapidly dissolving and spreading out.

The dirt emitters share the same 2D billboard particle output, but have a weaker gravity effect and are set to expand in a ring by default, making it more suitable for footstep/dirt clouds. 

Dirt emitters have a burst output, so all particles shoot out at once as if debris is kicked up from sudden impact. 

## 2D Stylized Smoke & Dirt Clouds

These emitters are a more stylized version of the previous 2D emitters. They share the same NS structure but values are adjusted to fit the new stylized 2x2 hand-drawn textures they use. 

A slight sprite rotation rate is applied to create move motion from these static sprites, and the size scaling curve has been tuned for the stylized textures. Particle spawn rates also had to be lowered to prevent the hand-drawn shapes from overlapping and washing out each other.

The result is a cleaner, hand-drawn effect compared to the more organic procedurally textured effects.

# Showcase Setup

The emitters are set up in the Lvl-Showcase\_Materials map. I created 12 emitters for display, 6 uses the base NS version, while the other 6 are configured in the **Details\>Effects** panel of each NiagaraActor into example versions to showcase the modularity of each effect. 

The 12 emitters are organized into 3 rows, a Steam, Smoke, and Dirt row from back to front. Each emitter is named and organized in the Outliner into folders, under the root GasEmitters folders. 

# How to Use

To use, simply drag the Niagara System effect from **Content Drawer \> ProjectAtlantis/Effects/Gases**. You will see each of the 6 emitters there. 

Then you can click on the effect in the level and navigate under **Details\>Effects\>User Parameters** to see a list of user parameters to adjust. Adjusting these parameters here will not affect the original, or other niagara systems so feel free to experiment.

## User Parameter Explained

Not all parameters listed below will appear on every emitter, some parameters have been omitted on individual emitters to avoid confusion. (For example Spawn Rate will do nothing on a Burst emitter).

Spawn Setting

* **Looping:** Check this to enable the particle effect to loop continuously  
* **Loop Duration:** The time in seconds for each loop, make sure its above lifetime duration to prevent cycles from overlapping  
* **Loop Delay:** The time in seconds before the next loop starts after loop duration is complete  
* **Steam/Smoke/Dirt Color:** Changes the particles base color  
* **Spawn Rate:** Increase to make particles spawn more frequently over time for continuous emitters  
* **Spawn Count**: The total number of particles it spawns at once for burst emitters  
* **Lifetime Min:** The time in seconds for the random minimum lifetime of a particle  
* **Lifetime Max:** The time in seconds for the random maximum lifetime of a particle

Shape

* **Upward Only (toggle):** Enable this to prevent height spread from entering negative axis  
* **Height Spread:** Determines how many angles \+/- along the x axis it spreads to, so setting to 90 will mean a height spread of \+90 & \-90, creating a vertical semi circle as output shape  
* **Width Spread:** Determines how many angles \+ along the Z axis it spreads to. So setting to 360 will mean a full circular width spread, creating a ring as output shape

Velocity

* **Drag:** Increase to make the particle slow down faster  
* **Gravity:** Increase to make the particle float upwards faster  
* **Velocity Min:** The minimum random initial velocity of the particle  
* **Velocity Max:** The maximum random initial velocity of the particle