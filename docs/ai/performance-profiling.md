# AI Runtime Performance Profiling

The JH AI has two complementary outputs for understanding per-frame cost:

- **`ai_performance.csv`** — a rolling per-window average log written during
  gameplay, produced by `AIPerfReporter`.
- **Shutdown summary** — a one-time table sorted by total CPU time, printed to
  stderr when the process exits, produced by `AIRuntimeProfiler`.

Both sources draw from the same instrumentation: `ScopedAIRuntimeProfile`
objects placed inside `AIPlayerJH::RunGF()` and its callees.

## Enabling profiling

Profiling is opt-in. Set the environment variable before launching the game or
`ai-battle`:

```bash
RTTR_AI_PROFILE=1 ./s25client
RTTR_AI_PROFILE=1 ./ai-battle ...
```

Without the variable the profiler accumulates no data and all CSV columns show
zero.

## Instrumentation

Each timed code block wraps itself in a `ScopedAIRuntimeProfile`:

```cpp
{
    const ScopedAIRuntimeProfile profile(AIRuntimeProfileSection::ExecuteAIJob);
    ExecuteAIJob();
}
```

The constructor records `steady_clock::now()`; the destructor calls
`AIRuntimeProfiler::AddSample()` with the elapsed nanoseconds. The profiler is
a process-lifetime singleton (`AIRuntimeProfiler::Instance()`).

### Instrumented sections

Defined in `AIRuntimeProfileSection` (`AIRuntimeProfiler.h`):

| Section | What it covers |
|---|---|
| `RunGF` | Entire `AIPlayerJH::RunGF()` body after the perf reporter call |
| `RefreshBuildingQualities` | Map BQ refresh every frame |
| `BuildingPlannerUpdate` | Planner state update every frame |
| `ExecuteAIJob` | Job queue dispatch, runs every 100 GF |
| `ExecuteEventJobs` | Event-driven job batch inside `ExecuteAIJob` |
| `ExecuteConstructionJobs` | Construction job batch |
| `ExecuteConnectJobs` | Road-connect job batch |
| `ExecuteGlobalBuildJobs` | Global position search jobs |
| `ExecuteBuildJobs` | Standard build jobs |
| `EvaluateCaptureRisks` | Capture-risk pass, runs every 500 GF |
| `TryToAttack` | Attack decision logic |
| `TrySeaAttack` | Sea attack decision logic |
| `CheckEconomicHotspots` | Expedition/forester/mine checks, every 1500 GF |
| `UpdateTroopsLimit` | Troop limit recalculation, every 1000 GF |
| `AdjustSettings` | Distribution/tool setting adjustments, every 150 GF |
| `PlanNewBuildings` | Building placement planning |
| `SelectAttackTargetAttrition` | Attrition target selection |
| `AttritionGetPotentialTargets` | Candidate target enumeration |
| `AttritionRecaptureScan` | Recapture opportunity scan |
| `AttritionPickRecapture` | Recapture target selection |
| `AttritionForceAdvantageCheck` | Force-advantage gate |
| `AttritionNearTroopsDensityCheck` | Nearby troop density check |
| `AttritionFallbackBiting` | Fallback biting attack |
| `UpdateTroopsLimitScan` | Border building scan inside troops-limit |
| `UpdateTroopsLimitScore` | Scoring pass |
| `UpdateTroopsLimitDistribute` | Distribution pass |
| `UpdateTroopsLimitApply` | Limit application pass |

## `ai_performance.csv`

Written to `STATS_CONFIG.statsPath/ai_performance.csv`, appended each time
`gf % 1000 == 0` and the player is not locked.

### Column layout

| Column | Description |
|---|---|
| `PlayerID` | Zero-based index of the AI player that wrote this row |
| `GameFrame` | Current game frame |
| `ElapsedMillis` | Wall-clock milliseconds since the previous log row |
| `WindowGameFrames` | Game frames covered by this row (`gf − previous gf`) |
| `<Section>_AvgUsPerGF` | Average microseconds per game frame spent in this section during the window |
| `<Section>_AvgUsPerCall` | Average microseconds per invocation of this section during the window |
| `<Section>_Calls` | Number of times the section ran during the window |

Section columns are emitted for: `RunGF`, `RefreshBuildingQualities`,
`BuildingPlannerUpdate`, `ExecuteAIJob`, `EvaluateCaptureRisks`, `TryToAttack`,
`TrySeaAttack`, `CheckEconomicHotspots`, `UpdateTroopsLimit`, `AdjustSettings`,
`PlanNewBuildings`.

### Reading the data

- **`RunGF_AvgUsPerGF`** is the most direct late-game health signal: it shows
  how many microseconds of each frame the AI consumes on average.
- Infrequent sections (`EvaluateCaptureRisks`, `CheckEconomicHotspots`, etc.)
  show zero `Calls` in windows where they did not fire. Their `AvgUsPerCall`
  only reflects windows where they actually ran.
- The first row (GF 0) has `WindowGameFrames = 0` and all section values are
  zero — no AI work has been done yet when that row is written.
- The sum of child section `AvgUsPerGF` values will be lower than
  `RunGF_AvgUsPerGF` because not all sections are surfaced in the CSV and there
  is uncounted overhead between spans.

## Shutdown summary

When the process exits, `AIRuntimeProfiler`'s destructor prints to stderr:

```
AI runtime profile summary
section                      calls     total ms       avg us       max us     avg work     max work
RunGF                         5000      2341.2          468.2        891.5          0.0            0
ExecuteAIJob                    50       843.1        16862.0      24500.1          0.0            0
...

Global build jobs by building type
building                     calls     total ms       avg us       max us
Woodcutter                     120        12.3          102.5        310.0
...
```

Rows are sorted by `totalNs` descending. Sections with zero calls are omitted.
The global build job table is a separate breakdown of `ExecuteGlobalBuildJobs`
by building type, useful for finding which searches dominate late-game cost.

## Source files

| File | Role |
|---|---|
| `libs/s25main/ai/aijh/debug/AIRuntimeProfiler.h` | Section enum, snapshot types, profiler and scoped-guard declarations |
| `libs/s25main/ai/aijh/debug/AIRuntimeProfiler.cpp` | Accumulation, `GetSnapshot()`, shutdown summary |
| `libs/s25main/ai/aijh/debug/AIPerfReporter.h` | Reporter declaration |
| `libs/s25main/ai/aijh/debug/AIPerfReporter.cpp` | CSV header generation, per-window delta computation |
| `libs/s25main/ai/aijh/runtime/AIPlayerJH.cpp` | Span placement in `RunGF()` |
