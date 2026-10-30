# Clue grouping by district and mystery (#436)

Entries now carry optional `DistrictId`, `DistrictTitle`, `MysteryId` and `MysteryTitle` in the existing `FJournalInvestigationEntry`. No additional model store or subsystem is introduced.

`WBP_JournalInvestigationTab` groups disclosed clues under district and mystery headings using the reusable `WBP_JournalClueGroup`. It finds an existing child group by its stable ID, so equal public titles remain separate and the same mystery ID in different districts stays scoped to its own district. Group widgets contain heading text and a child panel only; the journal subsystem remains independent of presentation.

`RefreshClues` keeps the existing disclosure filter and delegates row placement to `AddGroupedClue`. The added Blueprint functions are `AddGroupedClue`, `FindOrAddClueGroup`, and `AddClueRow`. The existing row, description widget, shell layout, background, and open/close lifecycle are reused.

Groups are created only for disclosed entries. Entries lacking district or mystery identity retain the original flat-list view. Each refresh reads the journal's stable entry-ID order; group order is the order of the first visible entry in each group, independent of event delivery order. This is deterministic rather than alphabetical by heading.

## Placeholder review

During PIE, load the existing helper once:

`exec(open(unreal.Paths.project_dir() + 'Tools/Journal/seed_discovered_clue.py').read())`

Run `clear_clues()` and then `seed_grouped_clues()`. Expect District A containing Mystery One (two clues) and Mystery Two (one clue), followed by District B containing its own Mystery One (one clue). No hidden district, mystery or clue should appear. Repeat the seed command and reopen the journal; groups and clues must not duplicate. `clear_clues()` should remove all groups and restore the empty state.

Use the player-controller-owned journal for pause/input checks: the existing `L_JournalTest` level script creates an additional always-visible journal shell.

This is only the #436 increment. Revised clue meaning, puzzle observations and incompleteness remain later stories. All content here is explicitly placeholder material.

## Validation (2026-10-30)

- Development Editor / Win64 compilation passed with the temporary authoring helper removed.
- Both grouped-view Blueprints compiled and were saved. The shell and clue-row assets are unchanged by #436.
- Eight focused specs passed with zero engine-log errors: the original five discovered-clue regressions plus nested identity grouping with equal headings, hidden-group suppression/clear, and deterministic ordering across delivery orders.
- Rendered PIE showed two districts, three mystery groups and four disclosed clues. Re-seeding retained two district groups; clearing removed all groups and restored the empty state. Captures: local ignored `Saved/Issue436-grouped.png` and `Saved/Issue436-grouped-bottom.png`.
- Build/test evidence is in local ignored `Saved/Issue436`. As with #435, the MCP test summary returns zero parsed results; engine-log completions and exit 0 supply the actual evidence.
- Stop for human inspection before #437. No real narrative or puzzle logic is included.
