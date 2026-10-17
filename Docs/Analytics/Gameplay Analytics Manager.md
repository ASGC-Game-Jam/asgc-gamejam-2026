# Gameplay Analytics Manager

Issue #304. `UGameplayAnalyticsSubsystem` initializes automatically per GameInstance. It needs no placed Actor, Level Blueprint, or feature-specific timer. `UScenePerformanceAnalyticsSubsystem` remains the sole owner of GameAnalytics session initialization; the manager initializes that dependency first.

## C++ and Blueprint API

```cpp
#include "Analytics/GameplayAnalyticsSubsystem.h"

UGameplayAnalyticsSubsystem* Analytics = GetGameInstance()->GetSubsystem<UGameplayAnalyticsSubsystem>();
Analytics->RegisterMetric(TEXT("Example:Duration"), EGameplayAnalyticsAggregation::Average, 10);
Analytics->RecordValue(TEXT("Example:Duration"), 32.5f);
Analytics->RecordEvent(TEXT("Example:Action"));
```

In Blueprint, use Get Game Instance, Get Subsystem (GameplayAnalyticsSubsystem), then Register Metric, Record Event, or Record Value. Flush Analytics and End Gameplay Session are also Blueprint-callable. Recording only changes local aggregates. It never calls the SDK or saves on every observation. Unregistered events default to Count and numeric observations to Sum. Register first for Average or Latest. Re-registering with another strategy is rejected; priority is set on first registration.

Count reports occurrences, Sum reports the sum, Average reports sum / sample count, and Latest reports the last observation. Pending samples from multiple intervals are weighted by their sample counts, including across unavailable-provider intervals and UTC rollover. Once sent, each Average event is the mean of that particular snapshot: averaging those events on a dashboard does not produce a sample-weighted lifetime mean. Register independent stable Sum and Count IDs when downstream analysis needs a weighted global mean. Numeric values must be finite and representable by the SDK's float interface; an oversized pending sum remains local rather than submitting infinity.

Successful snapshots clear only their own observations; observations recorded during SDK submission remain pending. Calls and delegates run on the game thread; worker threads must dispatch recording to it.

## Identifiers and bounded storage

IDs have one to five nonempty colon-separated segments, each at most 32 ASCII letters, digits, underscore, hyphen, or period (164 characters including separators). Do not put player IDs, timestamps, coordinates, generated values, or arbitrary user input in identifiers. Registration validates syntax, not semantic stability. Producers own a fixed vocabulary. `Session:GameplayDuration` is reserved and cannot be registered as a periodic metric.

The persisted map contains at most 1024 registered identifiers. Repeated observations update fixed-size count/sum/latest fields; there is no event queue that grows with play time. Registration beyond capacity is rejected. Scene-performance IDs use the same character rules and truncate map segments to 32 characters. Distinct names that sanitize/truncate identically share a metric; choose short distinct map names. Existing startup, map-load, scene-ready and transition measurements now record Average milliseconds through this shared manager. Version 1 performance aggregates migrate from Sum to Average using their retained count and sum.

## Active time, sessions, and scheduling

Only the owning GameInstance's primary game world contributes time. Paused worlds, dedicated servers, MainMenu-named maps and the exact Entry map are excluded. Menu maps must follow this naming convention. The tick callback uses undilated engine frame time, so gameplay time dilation does not change the allowance. Menu and pause time do not advance the scheduler. Gameplay-to-gameplay map transitions checkpoint without ending the session; travel to MainMenu or Entry ends it. Producers can also call End Gameplay Session on an explicit normal exit. Repeated end/return/shutdown callbacks do not add or submit the same duration twice.

Every 300 accumulated active seconds, the manager computes the earned allowance. Fractional progress survives session boundaries and application restarts. Long simulated ticks are split at reporting boundaries. Candidates are ranked by active seconds since last submission plus priority * 30 seconds; priority is clamped to [-100, 100]. Stable ID ordering breaks ties. Aging eventually promotes lower-priority pending metrics ahead of frequently sent higher-priority metrics. New keys do not get an artificial age advantage.

| Active gameplay in UTC day | Periodic allowance |
| --- | ---: |
| First 4 hours (48 intervals) | 320 |
| Next 4 hours (48 intervals) | 150 additional |
| Session-duration summaries | 10 separately reserved |
| Total manager cap | 480 |

The allowance at interval n is floor(n * 320 / 48) in phase one, plus floor(n * 150 / 48) in phase two. Unused earned allowance remains available. Empty metrics cost nothing. Manual flushes, map transitions, and multiple sessions share the same persisted counters; they cannot create an allowance. A scheduled flush at exactly eight active hours can spend the final earned allowance. Once active time exceeds eight hours, periodic submissions stop until the next UTC day, while aggregation continues. Session summaries can still use their reserve.

`Session:GameplayDuration` reports active seconds. Excess sessions or provider failure leave duration pending, combined with later durations rather than maintaining an unbounded session list. Deferred summaries are retried on eligible flushes or another session end. An open session's checkpointed duration is recovered as pending on application restart.

The 500-event figure is a planning assumption from Issue #304, not a guarantee of billing or SDK behavior. The manager leaves 20 events outside its 480-event cap for legacy/SDK traffic. All C++ design-event submission paths in ProjectAtlantis route through the manager. Third-party SDK automatic health/error/session traffic and direct Blueprint calls to the SDK are outside this gate; future producers must use the manager.

## Persistence, rollover, and delivery limits

SaveGame slot `GameplayAnalyticsBudget` stores version 2 state: UTC day, consumed periodic/summary counters, day active time, interval progress, open/deferred session durations, metric strategies/priorities and pending aggregates. Editor instances use separate `_Editor_<PIE instance>` slots and never submit to the production SDK.

Active gameplay checkpoints at most once per minute, plus explicit flush, map transitions and shutdown. Registration/recording do not cause per-observation disk writes. Each successful submission additionally needs one checkpoint for duplicate prevention (at most 480 such reservations per reporting day). Disk writes are synchronous and bounded; a crash can lose observations since the last successful checkpoint. A normal shutdown checkpoints menu-recorded observations too.

Before calling the SDK, the manager durably reserves quota and removes the snapshot. If saving fails, it restores the pending data and quota and does not call the SDK. If SDK access fails, it restores that snapshot while preserving newer observations and stops the batch. A missing provider does not clear data. The existing plugin exposes a void enqueue method: calling it successfully is not proof of SDK validation, network transmission, or server delivery.

There is no atomic transaction between SaveGame and the SDK. This implementation favors preventing duplicate submissions across restarts: a crash after the reservation is saved but before SDK enqueue can lose that one snapshot and consume its allowance. It does not claim exactly-once delivery. Abrupt termination cannot guarantee a final summary. Simultaneous packaged application processes sharing the same save slot are unsupported.

At the first observation/tick/flush after a forward UTC date change, day counters, active time, interval progress and submission ages reset. Pending metrics and session durations remain and may report on a different date from their observations. Midnight precision is the next callback/frame; no historical per-day backlog is kept. Backwards clock adjustments do not replenish an already consumed reporting day. Known version 1 saves migrate; unreadable/unsupported saves and invalid budget counters fail closed for the current UTC day. Invalid metric entries are removed and restored maps are bounded. SaveGame is best-effort local storage, not tamper-resistant quota enforcement.

## Validation

Build with Unreal 5.7.4:

```text
Build.bat ProjectAtlantisEditor Win64 Development -Project=<absolute ProjectAtlantis.uproject> -WaitMutex -NoHotReloadFromIDE -NoUBA -NoDebugInfo
```

`-NoUBA -NoDebugInfo` were necessary on the validation host to avoid local build-accelerator permissions and an MSVC D8050 debug-record failure. They change generated build output only. The editor target compiled successfully.

Run `Automation RunTests Atlantis.Analytics.Manager` in Unreal's automation console, or use UnrealEditor-Cmd with `-unattended -nullrhi -ExecCmds="Automation RunTests Atlantis.Analytics.Manager" -TestExit="Automation Test Queue Empty"` and a report directory. The seven passing cases are Aggregation, BudgetAndFairness, FailureAndReentrancy, RolloverAndSessions, Persistence, BoundsAndBlueprint, and WorldLifecycle. They use a private fake SDK sink and accelerated time; Persistence also saves/reloads a unique real disk slot and removes it afterward. WorldLifecycle verifies automatic initialization, actual world pause/resume filtering, simulated travel callbacks, and shutdown deduplication. Blueprint-callable flags are checked through Unreal reflection. These tests do not prove server delivery or replace interactive travel through authored maps.

MCP inspection is unavailable on this host: the MCP server points to another project and the editor plugin is offline. No binary assets were edited. Packaged-build results and remaining validation limitations are recorded in the task's final report.

The final validation run on 2026-10-17 passed all 17 tests in the full Atlantis editor group with zero warnings/failures. The same seven analytics cases also passed in the packaged Windows Development executable, with zero warnings/failures; the packaged log confirmed normal GameAnalytics session initialization. The analytics cases support both EditorContext and ClientContext and inject provider-unavailable behavior rather than relying on the editor-only SDK guard.

A minimal Windows Development package was built/cooked/staged successfully with `/Engine/Maps/Entry`. The initial cook reported six existing text-localization warnings in credits assets and no errors; assets were not resaved. Rebuild/restage used the cooked content after the final source changes. Packaged tests used `-nullrhi`, a separate `-UserDir`, and an injected SDK sink. This verifies runtime initialization, reflected API access, manager behavior and disk serialization in a cooked executable, but not actual dashboard delivery or interactive gameplay through authored maps.

For manual completion, use a normal packaged gameplay session lasting over five active minutes, pause/resume, travel between gameplay maps, return to the main menu, and restart. Check the GameAnalytics dashboard for scene-performance events and `Session:GameplayDuration`; pause/menu time should not contribute to active duration. Dashboard receipt, graphics-enabled play and actual cross-process recovery of a deliberately interrupted session remain manual checks.
