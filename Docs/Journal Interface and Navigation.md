# Alpha Journal interface and navigation

Engineering completion for #116 and verification of #298, targeting **Unreal Engine 5.7.4**. The requirements are the interface/navigation specification in #116 and the controller-access reproduction in #298. PRs #283 and #299 already merged the authored shell and gamepad bindings; this change extends those assets without replacing or reparenting them.

## Existing controls and lifecycle

`BP_AtlantisPlayerController` creates the existing `WBP_Journal` during its Blueprint BeginPlay. `IA_Journal`'s Started binding calls the shell's existing `HandleJournal`. Both `IMC_Default` and `IMC_Swim` map it to **Tab** and **Gamepad Special Right**. The actual action asset enables triggering while paused, allowing the same action to close the Journal.

The existing Blueprint `ShowJournal`/`HideJournal` path owns adding/removing the shell, game pause, cursor visibility, Game and UI input, and Game Only restoration with input flushing. No second pause policy or input allocation is introduced. Native controller mapping contexts remain available on close.

The Alpha shell already contains Investigation, Puzzle and Dialogue buttons. Their existing Blueprint handlers remain responsible for section selection and highlighting. Map presentation, entry data and final visual polish are separate scopes.

## Empty states

The merged widget assets had tab labels and investigation content but no empty-state copy. `UAtlantisJournalEmptyStateExtension` inserts one localized text widget into the authored content column. It uses the existing buttons and `CurrentTabType` property; it does not introduce a second navigation model.

The native player controller installs the extension after `Super::BeginPlay()` has run the authored Blueprint's Journal creation. It does not poll for widgets or modify the binary assets. On construction the extension binds existing button and Journal data notifications; on destruction it removes its text and bindings so reopening is safe.

Investigation shows **No investigations discovered yet.** when there are no discovered legacy entries, including when only Undiscovered placeholders exist. A discovered entry hides the message; clearing entries restores it. Puzzle and Dialogue currently show **No puzzle entries yet.** and **No dialogue entries yet.** because their entry presentation is outside #116. Future entry views must replace or suppress these messages when they display real content; #142/#144 are not pulled into this foundational change.

## Executed verification on October 19, 2026

Development Editor / Win64 built successfully using installed Unreal 5.7.4, with no build-environment override. `Automation RunTests Atlantis.Journal.AlphaShell` passed **all three specs with zero failures and zero test warnings** under UnrealEditor-Cmd, `/Engine/Maps/Entry`, `-unattended -NullRHI -nosound` and `-TestExit="Automation Test Queue Empty"`.

The isolated runtime fixture initializes actors and a real local player/viewport, loads `BP_AtlantisPlayerController`, runs its authored BeginPlay and executes the actual Enhanced Input Journal Started binding. The cases verify:

- Journal visibility, pause/resume, cursor changes, Game and UI / Game Only restoration, preserved desired mapping contexts and the mapping still applied in the real Enhanced Input subsystem.
- Existing three-button section selection, readable empty states, Undiscovered filtering, discovery/clear updates, and close/reopen lifecycle.
- Actual `IA_Journal`, `IMC_Default` and `IMC_Swim` assets, both exact keyboard/gamepad mappings, and the action's trigger-while-paused flag.

A separate read-only Unreal 5.7.4 Python inspection confirmed the authored WidgetTree contains no empty-state labels, the three existing section buttons/content hierarchy, the exact Tab/Gamepad Special Right mappings, and `trigger_when_paused=true`. The original content uses a Fill slot; the added text uses an Auto slot in the same column. No binary assets, Blueprint graph exports, art or gameplay data were changed.

These headless cases exercise the real assets and runtime binding, rather than connected controller hardware or graphics rendering. Physical gamepad navigation/focus, screen layout at target resolutions and a packaged gameplay smoke test remain interactive checks.
