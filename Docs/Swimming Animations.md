# Player swimming animations

`BP_AtlantisPlayerCharacter` uses `ABP_Player` in `Characters/Common/Animations/Swimming`. This Blueprint preserves the existing `ABP_Unarmed` land graph and adds two looping sequences after its land pose output. Other characters using `ABP_Unarmed` are unaffected.

`UAtlantisPlayerAnimInstance` reads the character movement component each animation update:

- Swimming movement mode selects the underwater branch.
- Active movement input with speed above 3 cm/s selects `A_Swim_Fwd`.
- No movement input selects `A_Swim_Idle`, including passive Ascend/Descend ballast drift and residual velocity after releasing input.
- Leaving swimming movement mode blends back to the original land graph, including falling, jumping, and walking.

Both pose transitions blend over 0.2 seconds. Swimming bypasses the land foot IK rig. No movement mode, ballast settings, input mappings, or collision settings are changed.

See `SourceArt/Animations/Swimming/README.md` for source attribution, import settings, and retarget configuration.

## Verification

Development Editor / Win64 build passed. `Atlantis.Animation.SwimmingTransitions` verifies land movement, water entry, idle, passive ballast ascent/descent, active horizontal/vertical swimming, input release, exit, and re-entry. It also evaluates the animation graph to check the distinct forward-swimming pose and the upright land pose after exit. Saved asset checks verify the player assignment, matching Quinn skeleton, looping graph nodes, positive clip durations, and disabled root motion.

An automated Play In Editor session in `L_Tutorial_Main`, with rendering enabled, passed checks for entering the actual water volume, underwater idle, active swimming, input release, exiting the volume, and re-entry. The session used the player Blueprint and the tutorial's existing ballast water volume.
