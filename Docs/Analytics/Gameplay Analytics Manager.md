# Gameplay Analytics Manager

Issue #304. The manager is a `UGameInstanceSubsystem` instantiated without any placed Actor or Blueprint.

## Producer API

```cpp
UGameplayAnalyticsSubsystem* Analytics = GetGameInstance()->GetSubsystem<UGameplayAnalyticsSubsystem>();
Analytics->RegisterMetric(TEXT("Ballast:Duration:Ascend"), EGameplayAnalyticsAggregation::Sum, 0);
Analytics->RecordValue(TEXT("Ballast:Duration:Ascend"), 32.5f);
Analytics->RecordEvent(TEXT("Ballast:Entered:Ascend"));
```

`RegisterMetric`, `RecordValue`, `RecordEvent`, `FlushAnalytics`, and `EndGameplaySession` are Blueprint-callable. Recording never submits to the SDK. Unregistered count metrics default to Count, and unregistered numeric metrics to Sum. Register first to select Average or Latest. An identifier's aggregation strategy cannot change.

## Aggregation and reporting

Count sends the number of occurrences, Sum sends the numeric sum, Average sends the average, and Latest sends the most recent observation. Each metric sends one fixed event ID and one numeric value. Unique dynamic identifiers, coordinates, timestamps, and player IDs must not appear in the ID. IDs contain up to five colon-separated segments, each at most 32 alphanumeric/underscore/hyphen/period characters.

The scheduler accumulates active gameplay seconds on the world post-actor-tick callback; paused gameplay and the MainMenu/Entry maps are excluded. Every 300 active seconds it submits selected pending metrics. Priority and time since the last send determine selection. The SDK accepts event values asynchronously, so local enqueue is **not** guaranteed server delivery.

Daily planning threshold: 500 GameAnalytics events per active user. The manager uses 480, leaving a 20-event margin. Up to 320 periodic events are earned across gameplay hours 0–4 and 150 across hours 4–8, plus 10 session summaries. Event allowance is earned proportionally at each five-minute interval. At eight hours, periodic sends stop; observations remain aggregated until the next UTC day. Each session end attempts `Session:GameplayDuration` with total active seconds; if its reserved quota runs out, the duration remains pending for a later eligible submission.

State persists in the `GameplayAnalyticsBudget` SaveGame slot, including pending aggregates and daily budget. The active gameplay budget resets at UTC date rollover while pending aggregates remain. Storage is bounded to 1024 identifiers. The manager is intentionally limited to single-player/local telemetry; producers should not record remote-player observations.

## Integration and limitations

`UScenePerformanceAnalyticsSubsystem` owns GameAnalytics session initialization. Other features should never call `UGameAnalytics` directly; they should record through this subsystem. Design-event reporting is excluded in editor builds to match the pre-existing analytics behavior.

This is application-level best-effort persistence and SDK enqueue, not guaranteed delivery. A crash can lose unsaved in-memory observations; process shutdown cannot guarantee a final event. Averaged or latest samples lose per-observation details, by design. The existing scene-performance event path should be routed through the manager so its submissions share the budget.

## Packaged build verification

1. Start a packaged build and trigger several `RecordEvent` and `RecordValue` calls from a test Blueprint or temporary test producer.
2. Confirm no event is sent immediately, and an interval submission uses the documented fixed IDs.
3. Switch between several metrics, pause, resume, and transition maps. Check active-gameplay accounting and session-duration reporting.
4. Restart and verify pending data/daily counters persist.
5. Inspect the GameAnalytics dashboard for the expected event IDs, using the SDK's normal reporting delay.
