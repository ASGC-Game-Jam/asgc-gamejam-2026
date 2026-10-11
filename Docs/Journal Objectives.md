# Journal Objectives

Issue #144 adds a read-only Objectives view. `UAtlantisJournalObjectivesWidget` is embedded by the Journal shell and refreshes on `OnObjectivesChanged`. It shows the supplied objective state and text, then optional context and location. It has no completion controls, clue inspection, rewards, markers, or puzzle solution generation.

## Ownership and integration

Objective State & Progression (#190, currently open) owns objective state. No implementation of that owner or authored Beta objectives exists in the current source/Blueprint exports. Until that dependency lands, the Blueprint-callable `UAtlantisJournalObjectivesSubsystem::SubmitObjectiveSnapshot` is the explicit presentation adapter. The owner must submit its full ordered snapshot of **disclosed player-facing objectives** after initial load and whenever state, text, context, or location changes. Do not submit hidden objective data or internal solution text; presentation cannot infer which authored text is safe to disclose.

The adapter validates nonempty IDs, unique IDs, nonblank objective text, and supported states before replacing anything. It returns false on invalid input and retains the previous snapshot. An empty snapshot removes all entries. Stable IDs identify records; owner order determines display order. Snapshot getters return copies. There is no state transition API or gameplay persistence here.

Completed and failed objectives are hidden by default. Design can call `SetHistoryVisibility` to expose either independently. The snapshot keeps these records so toggling presentation policy loses no data. These defaults are a reversible conservative first-pass policy pending Design approval. Location is displayed only when explicitly provided by the owner; the Journal adds no precision, coordinates, or map markers.

The subsystem follows the existing `UAtlantisJournalSubsystem::OnJournalCleared` event for run/menu reset. On load the objective owner should republish its current disclosed snapshot, avoiding a competing quest save state. Closing/reopening the widget retains presentation data; NativeConstruct subscribes and renders the current snapshot, and NativeDestruct unsubscribes.

## Existing implementation

The existing `WBP_Journal` handles Investigation, Puzzle, and Dialogue tabs, next-tab navigation and notification blinking. It has no Objectives tab. This change supplies a native Objectives view without modifying binary widgets or exported graphs. The shared shell integration supplied alongside #142 must preserve access to the existing Journal tabs; `CreateWidget<UAtlantisJournalObjectivesWidget>` creates the view without an asset dependency.

## Validation

Automation test `Atlantis.Journal.ObjectivesSnapshot` covers supplied state and text, optional fields, independent completed/failed visibility, stable order, mutation isolation, atomic rejection, removed entries, and run reset. Build `ProjectAtlantisEditor` Development Editor / Win64, then run the test in the Unreal Session Frontend or via `UnrealEditor-Cmd -ExecCmds="Automation RunTests Atlantis.Journal.ObjectivesSnapshot;Quit" -unattended -NullRHI`.

For PIE: open the Journal and select Objectives; submit an active snapshot through the owner adapter; confirm updates while open, completion/failure visibility under each policy, blank optional-field omission, and a long scrollable localized description. Close/reopen and confirm the latest snapshot, then invoke legacy Journal `ClearAll` and confirm no old entries remain. Check the existing Investigation, Puzzle and Dialogue controls still work. Real Beta objective validation remains blocked by #190 and authored objective data; synthetic examples are not a substitute.
