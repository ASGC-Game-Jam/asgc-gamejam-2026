Water Leak Effects

By Malachi Choi

​

**Introduction:**

This document covers the two Niagara Systems I added to the project: WaterDrop and WaterSpray. Both are prototype initial implementations of these effects and can be easily customized or entirely replaced in the future. For now, they serve as water leakage for prototyping visuals.

The WaterSpray system was created following some information from this YouTube guide by Peyton Varney: [https://www.youtube.com/watch?v=jr4qrJ-dy5M.](https://www.youtube.com/watch?v=jr4qrJ-dy5M)

I then made the WaterDrop system by playing around with the parameters from the previously made WaterSpray system.

​

**NE_WaterFountain**

​

This is the initial Niagara Emitter that looks like a water fountain with water spraying out in an arc up from the center. This emitter isn't used as an effect; it's the base for all systems that can be customized based on the water effect you want.

​

**NS_WaterDrop_System**

​

The WaterDrop system replicates the slow drip of a pipe or something dripping from the ceiling, showing an occasional water droplet followed by a few others falling every few seconds.

When opened, the system has 2 variations of the WaterFountain Emitter; one can be disabled to have fewer water droplets falling periodically, or adjusted to have differences in the droplets.

​

The main parameters and changes made to the base NE_WaterFountain to make these effects are:

- **SpawnRate**
  - The low spawn rate of 0.856 makes the dripping infrequent.
- **Lifetime Min**
  - The lifetime min of 1.6 and max of 3.0 lets droplets exist long enough to fall and be noticed, but not so long that they build up too many particles, impacting performance.
- **Sprite Size**
  - The non-uniform sprite size has an X of 0.63 and a Y of 3; this makes the water droplet sprite more uneven and more like a water droplet than a circle.
- **Gravity**
  - Disabling gravity allows a water droplet to fall straight; it can be re-enabled if used at an angle.

**NS_WaterSpray_System**

​

The WaterSpray system is an effect meant to replicate a bursting pipe or broken glass with water rapidly spraying out from the leak.

​

The main parameters and changes made to the base NE_WaterFountain to make these effects are:

- **SpawnRate**
  - The high spawn rate of 500 lets lots of particles spawn, making it look like frequent bursts of water.
- **Lifetime Min & Max**
  - The low lifetime min of 0.1 and max of 0.196 ensures the spray doesn't last long enough to build up unneeded particles, impact performance, or create a fountain shape.
- **Sprite Size**
  - The non-uniform sprite size has an X of 3 and a Y of 0.5; this makes the water droplet sprite more uneven and more like a water droplet than a circle.
- **Velocity Mode**
  - The velocity mode is set to cone, which creates an arcing effect and forms a cone-like shape from the particle spawns, making it look like a more natural spray instead of a straight buildup.
- **Velocity Speed**
  - The velocity speed is set to a minimum of 200 and a max of 565; this speed lets the water particles emulate bursting sprays with a faster, more intense animation. The difference between the min and max allows more variation and interesting differences in each particle.
- **Gravity**
  - Gravity is enabled to let the spray arc toward where gravity pulls it near the end of the particle's lifetime.

**Use/Testing:**

​

The Niagara Emitter **NE_WaterFountain** and both Niagara Systems **NS_WaterDrop_System** and **NS_WaterSpray_System** can be found in the "WaterLeak" folder: Content\\ProjectAtlantis\\Effects\\WaterLeak

​

To place and use the Niagara Systems, drag them into the world, then use the move and rotate tools to position them however you like, like any asset or blueprint. Do note that the WaterDrop system will have to be rotated to have the droplet go down, as the base position starts with the droplet floating up so that it could be used for those purposes as well; the same is true for the spray system; it will likely have to be rotated to look right.

​

If you want to make a variation of the water leak effects, make an empty Niagara System and drag NE_WaterFountain into that system's graph; from there, you can tweak the parameters to shape the look.

​

If you would like to see an example of how the Niagara Systems can be used and look like, find the "Lvl_Showcase_Materials" level located in Content\\ProjectAtlantis\\Maps

​

Once inside the level, navigate to the Water Leaks showcase stage; you should find some prototype pipes, a mannequin, and the effects.

​

**End Note:**

​

If you would like any help with these systems, please reach out to mollunn on Discord or pop into the art channel and @ me!