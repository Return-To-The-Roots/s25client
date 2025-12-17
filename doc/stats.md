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

`saveStats` is invoked per game frame for the local AI player (unless the
player is locked). It opens/creates several CSV files inside
`STATS_CONFIG.statsPath`, appending rows keyed by the current game frame.

- On the first frame (`gf == 0`) the CSVs receive column headers derived from
  the English resource name arrays (`BUILDING_NAMES_1`, `JOB_NAMES_1`).
- Building counts: `GetBuildingsMap/Site/WantedMap` query the `BuildingPlanner`
  to capture constructed buildings, sites, and desired extras.
- Productivity: `GetProductivity` values per building type are appended to
  `productivity.csv`.
- Player snapshots: `stats.csv` is generated via `DataExtractor`, so each row
  mirrors the tool-facing snapshot format (`GameFrame`, `PlayerId`, statistics
  counters, buildings/sites/prod, goods, etc.).
- "Other" metrics (in `other.csv`): number of military buildings, available
  wood/stone from AI surface scans, current boards demand, average build time,
  and the soldier pool per `SOLDIER_JOBS` (sourced from inventory/job counts).

Average build time is derived by scanning all completed buildings since the
previous stats frame and measuring `GetBuildStartingFrame` to
`GetBuildCompleteFrame`. `lastStatsFrame_` stores the checkpoint to avoid
double-counting.

## Extended dumps every 2500 frames

Every 2,500 frames the AI emits a verbose text report (`ai_test_%010d.txt`)
containing:

- Timestamp, player name, and summarized scoreboard values.
- Resource availability, tool priorities, per-good counts, and building table
  entries showing counts, open sites, wanted count, and productivity.
- Job inventory and additional metrics (military building count, boards demand,
  average build time, soldier ranks).

These dumps supplement the CSV series with an immediately readable snapshot
for manual analysis.

Together these systems provide a holistic view of how AI players expand,
balance their economy, and perform in combat without attaching a debugger.
