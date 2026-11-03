# Release Engineering Test Plan

This focused branch carries the #144 section from the local v0.1.2 integration plan, adapted for its #116 prerequisite. Other release features remain in their own branches. Knowledge-view integration is optional and is covered by the combined branch; the focused coexistence test requires only Objectives.

## #144 — Objective Journal presentation

Grounding: `AtlantisJournalObjectivesSubsystem.h/.cpp`, `AtlantisJournalObjectives.spec.cpp`, existing shell layout tests, and the focused Alpha empty-copy coexistence regression.

Fixture: actual Journal shell with Objectives button; obtain `UAtlantisJournalObjectivesSubsystem` in a transient harness. Prepare owner-supplied `FJournalObjectivePresentation` values with unique IDs, readable Text, optional Context/Location, and Active/Completed/Failed states. No live quest progression owner is assumed by this fixture.

1. Open Objectives with an empty snapshot. Expected: its own readable empty state. Selecting it preserves the authored tab enum and hides other sections; no stale "No dialogue entries"/"No puzzle entries" copy appears beside it.
2. Submit an Active objective with context/location while the view stays open. Expected: immediate display of exactly the supplied public text, in owner order. The Journal must not infer a destination, quest state, or completion condition.
3. Replace the full snapshot using the same ID and revised text/state. Expected: old text/removed optional fields disappear. Completed/Failed items are hidden under default policy. Toggle `SetHistoryVisibility(true,false)`, then `(false,true)`, then `(true,true)`: only requested supplied history is shown, without changing cached owner state.
4. Submit duplicate IDs, missing IDs, whitespace-only text, and an invalid state. Expected: atomic rejection; the prior accepted visible/cache state remains intact. Modify a getter result copy and confirm the stored objective is unchanged.
5. Submit an empty snapshot and exercise `ClearObjectives` / legacy `ClearAll`. Expected: stale active/history rows disappear and the open native view shows its own empty state. No extra legacy empty text overlaps it.
6. Switch Objectives → Puzzle/Investigation/Dialogue, including returning to the same legacy enum value. Close and reopen Journal; destruct/reconstruct the extension and construct the older empty-state extension after native content. Expected: one Objectives button, correct section visibility, preserved original content parent, no duplicate subscriptions, unchanged pause/input/cursor behavior, and restored fallback when native sections are removed.

Automation: `Automation RunTests Atlantis.Journal.ObjectivesSnapshot+Atlantis.Journal.ObjectivesViewUpdates+Atlantis.Journal.ExistingShellLayout+Atlantis.Journal.ExistingShellExtension+Atlantis.Journal.AlphaShell`.

Cleanup/limits: clear objective snapshots/history policy and close Journal. #144 stores/displays owner-supplied snapshots; it does not implement the upstream objective/quest state system or a save-file owner. A full objective-to-gameplay walkthrough requires that separate producer to submit actual outcomes. Rendered presentation remains a manual check.

## Current validation

On 3 November 2026, UE 5.7.4 Development Editor / Win64 build passed (42.98 seconds). `ProjectAtlantis.Journal+Atlantis.Journal` completed 14 tests: 14 passed, zero failures, warning passes, skips or in-process cases; process exit 0. This includes snapshot validation/history/reset, live view updates, actual Blueprint layout/navigation, upstream disclosed-entry behavior, #116 lifecycle/gamepad mappings and the new native-objective/legacy-empty-state construction-order regression. Evidence: `Saved/Logs/Issue144ResumeBuild.log`, `Saved/Logs/Issue144ResumeTests.log`, `Saved/Automation/Issue144Resume/index.json` in this worktree. These generated files are intentionally uncommitted. The explicit engine commands target this isolated worktree because MCP build/test tools are bound to the primary checkout. No binary assets were authored or modified; existing Blueprint assets loaded and executed in the suite. Rendered PIE, physical gamepad navigation, packaging and real Beta objective walkthrough have not been rerun. #190 integration and Design confirmation of completed/failed visibility remain outstanding; synthetic fixtures do not satisfy real Beta acceptance.
