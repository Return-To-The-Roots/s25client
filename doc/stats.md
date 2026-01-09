# AIPlayerJH Statistics Logging

`libs/s25main/ai/aijh/AIPlayerJHStats.cpp` extends the JH AI with a telemetry
pipeline that captures both long-term economy metrics and short-lived combat
outcomes. The code is organized around three main areas: combat tracking,
per-frame CSV dumps, and less frequent human-readable snapshots.

## Combat tracking & logging

- `TrackCombatStart` registers every initiated attack: it records the target
  position/object, defender ID, and building type while seeding the
  `CombatLossTracker` with the target object ID and capture risk.
- `LogFinishedCombats` runs every frame. It inspects `activeCombats_`, uses
  `EvaluateCombatState` to decide if each combat succeeded, failed, or is still
  pending, and flushes finished entries to `combats.txt` under
  `STATS_CONFIG.statsPath`.
- Log lines include attacker/defender player indices, the targeted building,
  outcome, formatted per-rank force/loss breakdowns (`FormatRankCounts`),
  capture-risk percentage, and for successful assaults the destroyed-building
  list (`FormatDestroyedBuildings`).
- `InitializeCombatsLogFile` writes a one-time header describing active
  players via `FormatPlayerLabel`, ensuring analyst logs start with roster
  metadata.

## Periodic CSV snapshots

`saveStats` now focuses solely on `stats.csv`. Every time
`IsStatsPeriodHit(gf, STATS_CONFIG.stats_period)` evaluates true (and the player
is not locked), it uses `DataExtractor` to capture a snapshot for the local AI
and append it to the CSV. Header rows are emitted only once when
`statsSnapshotHeaderWritten` is still false.

## Debug CSVs & derived metrics

`saveDebugStats` drives the remaining CSVs and textual dumps whenever
`IsStatsPeriodHit(gf, STATS_CONFIG.debug_stats_period)` succeeds.

- On the first invocation it calls `InitializeStatsCsvFiles`, which emits
  headers for `buildings_count.csv`, `buildings_sites.csv`, `productivity.csv`,
  and `other.csv` using the English name arrays
  (`BUILDING_NAMES_1`, `JOB_NAMES_1`).
- Building counts: `GetBuildingsMap/Site/WantedMap` query `BuildingPlanner` to
  capture constructed buildings, sites, and desired extras.
- Productivity: `GetProductivity` values per building type are appended to
  `productivity.csv`.
- "Other" metrics (in `other.csv`): number of military buildings, available
  wood/stone from AI surface scans, current boards demand, average build time,
  and the soldier pool per `SOLDIER_JOBS` (sourced from inventory/job counts).

Average build time is derived by scanning all completed buildings since the
previous debug stats frame and measuring `GetBuildStartingFrame` to
`GetBuildCompleteFrame`. `lastStatsFrame_` stores the checkpoint to avoid
double-counting.

## Extended dumps every 2,500 frames

Every time `saveDebugStats` fires on a game frame divisible by 2,500 the AI also
emits a verbose text report (`ai_test_%010d.txt`) containing:

- Timestamp, player name, and summarized scoreboard values.
- Resource availability, tool priorities, per-good counts, and building table
  entries showing counts, open sites, wanted count, and productivity.
- Job inventory and additional metrics (military building count, boards demand,
  average build time, soldier ranks).

These dumps supplement the CSV series with an immediately readable snapshot for
manual analysis. Headless runs may additionally write minimap bitmaps to
`STATS_CONFIG.screensPath` whenever `IsStatsPeriodHit(gf,
STATS_CONFIG.minimap_period)` evaluates true, keeping visual state captures in
sync with debug metrics.

Together these systems provide a holistic view of how AI players expand,
balance their economy, and perform in combat without attaching a debugger.
