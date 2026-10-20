# Ballast selection on land (#288)

The authored `IMC_Default` now includes the same six fixed-state mappings as `IMC_Swim`: keyboard 2 / 3 / 4 and gamepad B / X / Y select Descend / Wander / Ascend. The native controller already owns those state-action callbacks and retains the selected PlayerState mode when movement contexts change. Land Jump, legacy swimming/breathe actions and movement bindings are preserved.

Requests still pass through existing control enable/suppression and authoritative PlayerState Oxygen checks. Choosing a mode on land does not bypass the Oxygen allocation requirement or force a movement mode.

`Scripts/AddLandBallastMappings.py` applies the narrow idempotent migration using Unreal Editor. Lock `Content/ProjectAtlantis/Core/Engine/Input/IMC_Default.uasset` through Git LFS before running it. It copies only existing fixed-state action mappings and rejects unexpected instanced triggers/modifiers. Keystone regenerates the exported representation on save.

Automation checks the actual authored controller/actions/mappings, state selection through EnhancedInput delegates, transition retention, Jump preservation, Oxygen rejection and control suppression.

Validation on October 20, 2026: Development Editor / Win64 built successfully with the branch's Unreal Engine 5.7 association. All nine `Atlantis.Input` checks passed against the fresh build, including the three land Ballast regressions and five control-suppression regressions. The authored land context was loaded by the tests. Graphical Play In Editor and packaged smoke tests were not run. Build and test logs are in `Saved/Logs/IssueValidationBuild.log` and `Saved/Logs/Issue288Validation.log` (generated, not committed).
