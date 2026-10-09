# Surface Walk (#47)

Surface Walk is native character custom movement: mode 1 is lower attachment,
mode 2 is upper attachment. `IsSurfaceWalking()` and
`GetSupportingSurfaceNormal()` expose attachment without adding another traversal
API. Open PR #265 owns typed traversal publication; its integration should map
these custom modes to Surface Walk rather than copying its unmerged API here.

While swimming in a water volume, Descend automatically attaches to a lower
blocking surface; Ascend attaches to an upper blocking surface. Wander cannot
attach. Changing away from the attachment's Ballast selection detaches immediately.
Support loss returns to Swimming while preserving the current Ballast state and
Oxygen reservation. Leaving water returns to native falling.

Compatible surfaces block the character capsule's collision channel. Both initial
contact and continued support must have an upward normal for Descend or downward
normal for Ascend within Character Movement's Walkable Floor Angle. Min Surface
Vertical Normal can tighten that slope limit. Walls and overly steep slopes cannot
become support, including when reached from an existing attachment. Supported
normal changes also respect Max Surface Normal Change Degrees.

Small uneven ledges are crossed using swept movement away from support, forward,
and back toward a valid surface, bounded by Max Step Height. This also works on
ceiling undersides. Capsule clearance is restored before reorienting, and overlapping
poses are rejected. Obstructed alignment or movement releases attachment to Swimming;
Blocked Surface Reattach Delay (default 0.2 seconds) prevents immediate retries
against the same obstruction so the player can move clear.
Attachment uses a capsule sweep, support uses a center trace, and movement uses
native swept collision and sliding. The capsule's up axis follows support.

Camera-relative forward and right inputs map into the surface plane, with right
preserved on inverted ceilings. Surface movement uses Max Walk Speed, Max
Acceleration, Ground Friction and Braking Deceleration Walking from Character
Movement; speed is independent of surface orientation. Normal swimming remains
owned by the existing swim physics. No Blueprint assets are modified.

Tune Surface Attach Distance, Surface Detach Distance, Surface Contact Offset,
Max Surface Normal Change Degrees and Min Surface Vertical Normal on the movement
component, plus Walkable Floor Angle and Max Step Height in Character Movement.
Distances are in centimetres. Attachment is immediate; no separate
input or reorientation delay is introduced.

## Validation

Development Editor / Win64 builds. Local runtime fixtures exercise lower and
upper attachment, wrong-side and Wander rejection, upright/inverted/sloped
orientation, camera-right ceiling movement, slope movement, equal speed limits,
support loss, selection preservation and synchronous Wander detachment. The final
audit also checks measured lower/upper speed equality, continuous normal changes
to steeper walkable support, water exit and editable attachment-distance behavior.
Wall/steep-slope rejection, 10 cm floor/ceiling seams and retreat from tall obstacles
are also covered by local regressions.
First Playable validation runs the actual player Blueprint in L_Tutorial_Main's
authored water with temporary lower/upper collision fixtures; no map changes are
saved. Network validation checks server attachment, client custom mode and
orientation, and replicated Wander detachment. Simulated proxies follow server
attachment; owners retain native movement prediction.

Validation sources remain local. Runs use NullRHI, so animation appearance and
movement feel still require visual review.
