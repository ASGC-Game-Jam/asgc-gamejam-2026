# Swimming animations

Source: [Quaternius Universal Animation Library](https://quaternius.com/packs/universalanimationlibrary.html), Standard pack. Licensed under CC0; see `License.txt`.

`UAL_Swimming.glb` contains the original rig, mesh, and only `Swim_Fwd_Loop` and `Swim_Idle_Loop` from the pack's in-place `UAL1_Standard.glb`. Unused animation data was removed. The root-motion variant is not used; the ballast movement component controls movement.

Import with Skeleton set to None, Import Animations and Import Bone Tracks enabled, Use 30Hz to Bake Bone Animation enabled, and Snap to Closest Frame Boundary enabled, following the supplied `Unreal_Setup.png`. Import materials and physics asset creation are disabled because the imported mannequin is only a retarget source.

The imported assets and the two IK rigs are in `/Game/ProjectAtlantis/Characters/Common/Animations/Swimming`. `RTG_UniversalSwimming_Quinn` maps the source to `SKM_Quinn_Simple`, using automatically generated humanoid chains and an aligned target pose. The pelvis motion operation has a global Z translation offset of 95.896782 cm, Quinn's reference pelvis height, to center the source's lowered swimming poses in the player capsule. This offset is baked into `A_Swim_Fwd` and `A_Swim_Idle`. Retargeted clips use Quinn's skeleton and have root motion disabled.
