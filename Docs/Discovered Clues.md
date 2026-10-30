# Discovered clues in the Journal (#435)

The existing `UAtlantisJournalSubsystem` owns the only entry store. `FJournalInvestigationEntry` now exposes a stable `EntryId`, localized `Title`/`Descriptions`, and the existing owner-supplied status to Blueprints. `SetInvestigationEntry` stores a snapshot under its identity and broadcasts the existing update event. The original title-keyed Blueprint functions remain available. Description-only legacy entries start undiscovered.

`WBP_JournalInvestigationTab` reads the model directly. Its Blueprint `RefreshClues` function filters out `Undiscovered`, creates `WBP_JournalClueRow` instances, and toggles the empty state. Construct binds the existing update/clear events and refreshes; Destruct removes those bindings. The row reuses the existing description Widget Blueprint. No native view, controller, polling, grouping, puzzle fields, or additional storage is introduced.

## Repeatable placeholder check

1. Open the project and start PIE on a playable map. `L_JournalTest` is available, but its existing level script also creates an extra always-visible journal shell; use the player-controller-owned journal for open/close checks.
2. In the Unreal Python console, run:

   `exec(open(unreal.Paths.project_dir() + 'Tools/Journal/seed_discovered_clue.py').read())`

3. Open the Journal Investigation tab using the existing input. Confirm one `[Placeholder] Discovered clue` and its description. The undiscovered placeholder must remain hidden.
4. Run the script again. Confirm one visible row, then close/reopen the journal repeatedly. Confirm normal pause, cursor and gameplay input restoration.
5. To check the empty state in the same Python session, run `journal.clear_investigations()`. This clears the current PIE journal, so use a disposable play session. Confirm `No clues discovered yet.`

The script is an explicit development stub, never seeded automatically into gameplay. It writes no assets. Real clue content and owning-system integration remain #441 validation.