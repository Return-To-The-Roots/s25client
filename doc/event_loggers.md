# Event Loggers Synopsis

This document summarizes the runtime event loggers currently available in `s25main`:

- `WareEventLogger`
- `BuildingEventLogger`
- `CombatEventLogger`
- `CountryEventLogger`

All loggers are gated by `STATS_CONFIG.statsPath`. If it is empty, no log file is written.
Text and protobuf event loggers buffer new records in memory and flush them when logging reaches the next
500-gameframe boundary, with a final flush during shutdown.

## WareEventLogger

### Purpose
Tracks inventory ware deltas per player.

### Hooks
- `GamePlayer::IncreaseInventoryWare(...)`
- `GamePlayer::DecreaseInventoryWare(...)`

### Output
- File: `wares_log.csv`
- Format: CSV
- Header:
  - `gameframe,playerId,goodtype,count`
- Semantics:
  - `count > 0`: ware added to inventory
  - `count < 0`: ware removed from inventory

### Notes
- Ware type is normalized through `ConvertShields(...)` before logging.
- `playerId` is written as 1-based (`playerId + 1`).

## BuildingEventLogger

### Purpose
Tracks building lifecycle events in a unified CSV stream.

### Events
- `construction_site_created`
- `construction_site_cancelled`
- `constructed`
- `destroyed`
- `captured`

### Hooks
- Construction site created:
  - `GamePlayer::AddBuildingSite(...)`
- Construction site cancelled:
  - `GamePlayer::RemoveBuildingSite(...)`
- Constructed:
  - Builder completion path in `nofBuilder`
- Destroyed:
  - `noBaseBuilding::Destroy()` (excluding `GO_Type::Buildingsite`)
- Captured:
  - `nobMilitary::Capture(...)`

### Output
- File: `building_log.csv`
- Format: CSV
- Header:
  - `gameframe,playerId,event,buildingType,buildingId,x,y`

### Notes
- A completed construction site is marked via `MarkConstructionSiteConstructed(...)` so it is not additionally logged as `construction_site_cancelled`.
- Construction-site events use `buildingId=0`.
- `playerId` is written as 1-based (`playerId + 1`).

## CombatEventLogger

### Purpose
Tracks combat-related AI and world events for debugging and analysis.

### Events logged
- Attack order (`ATTACK_ORDER`)
- Aggressive defender order (`AGG_DEFENDER_ORDER`)
- Fight result (`FIGHT`)
- Capture result (`CAPTURE`)
- Destroyed-building accumulation for capture context (`RecordCaptureDestroyed`)

### Hooks (representative)
- AI attack ordering in `GameWorld`
- Aggressive defender ordering in `nofAttacker`
- Fight resolution in `noFighting`
- Building capture in `nobMilitary`
- Building destruction attribution in `GameWorld`

### Output
- File: `combat_log.txt`
- Format: line-based text (not CSV)
- Typical line shape:
  - `#<gf> <EVENT_NAME> key=value ...`

### Notes
- Capture logging includes a destroyed-building summary gathered before capture finalization.

## CountryEventLogger

### Purpose
Tracks country-size changes per player.

### Hooks
- Territory recalculation in `GameWorld::RecalcTerritory(...)`
- Initial country-size snapshot after HQ placement at gameframe `0`

### Output
- File: `country_log.csv`
- Format: CSV
- Header:
  - `gameframe,playerId,change`

### Semantics
- `change > 0`: country size increased
- `change < 0`: country size decreased
- At gameframe `0`, the logger writes each active player's initial country size as a positive value.

### Notes
- `playerId` is written as 1-based (`playerId + 1`).
