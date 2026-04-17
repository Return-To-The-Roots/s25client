# Event Loggers Synopsis

This document summarizes the runtime event loggers currently available in `s25main`:

- `WareEventLogger`
- `BuildingEventLogger`
- `CombatEventLogger`
- `CountryEventLogger`
- `CountryPlotEventLogger`
- `MilitaryEventLogger`
- `RoadEventLogger`
- `TroopsLimitEventLogger`
- `ToolPriorityEventLogger`

All loggers are gated by `STATS_CONFIG.statsPath`. If it is empty, no log file is written.
When running `extras/ai-battle`, `--disable_event_logging` disables all event loggers and
`--enabled_event_loggers <names...>` restricts output to a subset. Supported CLI names are
`building`, `combat`, `country`, `country-plot`, `military`, `road`, `tool-priority`, `troops-limit`, and `ware`.
Text and protobuf event loggers buffer new records in memory and flush them when logging reaches the next
500-gameframe boundary, with a final flush during shutdown.

## WareEventLogger

### Purpose
Tracks inventory ware deltas per player.

### Hooks
- `GamePlayer::IncreaseInventoryWare(...)`
- `GamePlayer::DecreaseInventoryWare(...)`

### Output
- File: `wares_log.pb`
- Format: length-delimited protobuf stream of `WaresLogRecord`
- Semantics:
  - `delta > 0`: ware added to inventory
  - `delta < 0`: ware removed from inventory

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
- `inhabited`
- `destroyed`
- `captured`

### Hooks
- Construction site created:
  - `GamePlayer::AddBuildingSite(...)`
- Construction site cancelled:
  - `GamePlayer::RemoveBuildingSite(...)`
- Constructed:
  - Builder completion path in `nofBuilder`
- Inhabited:
  - `nobUsual::WorkerArrived()`
  - `nobMilitary::AddPassiveSoldier(...)` when the garrison becomes occupied
  - Warehouse construction completion in `nofBuilder`
  - `MapLoader::PlaceHQs(...)` for initial headquarters placement
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
- Construction-site events use the construction site's object ID as `buildingId`.
- `inhabited` is emitted when a building first counts as staffed: a worker for usual buildings, a stationed soldier for
  military buildings, and immediately on construction for warehouses. Initial headquarters placement is logged as
  inhabited immediately.
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
- File: `combat_log.pb`
- Format: length-delimited protobuf stream of `CombatLogRecord`

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

## CountryPlotEventLogger

### Purpose
Tracks exact territory plots acquired or lost by player.

### Hooks
- Territory recalculation in `GameWorld::RecalcTerritory(...)`
- Initial country snapshot after HQ placement at gameframe `0`

### Output
- File: `country_plots_log.pb`
- Format: length-delimited protobuf stream
- Stream layout:
  - one `CountryPlotsLogHeader`
  - followed by repeated `CountryPlotsLogRecord`

### Semantics
- Each record represents one territory-update event, not one row per player.
- Changed plots are grouped by `old_owner_id -> new_owner_id`.
- Plot positions are stored as packed delta-encoded row-major indices within a local bounding box.
- At gameframe `0`, the logger writes a full initial ownership snapshot as `0 -> player` transitions.

## MilitaryEventLogger

### Purpose
Tracks soldier inventory and movement events tied to military buildings.

### Events
- `rec`: recruit added
- `upg`: soldier upgraded
- `los`: soldier lost in combat
- `dep`: soldier deployed to a building
- `und`: soldier undeployed from a building

### Hooks (representative)
- Initial HQ soldier snapshot in `MapLoader`
- Recruit creation in `nobBaseWarehouse`
- Deployment, undeployment, and upgrades in `nobMilitary`
- Combat losses in `noFighting`

### Output
- File: `military_log.csv`
- Format: CSV
- Header:
  - `gameframe,playerId,event,rank,buildingType,buildingId,count`

## RoadEventLogger

### Purpose
Tracks road and flag lifecycle events relevant to logistics-network changes.

### Events
- Road constructed
- Road construction failed
- Road demolished
- Flag built
- Flag demolished

### Hooks (representative)
- Flag creation and destruction in `GameWorld`
- Road construction in `GameWorld::BuildRoad(...)`
- Road segment destruction in `noRoadNode`
- Road splitting in `RoadSegment`
- Building-front flag and access-road creation in `noBaseBuilding`
- Capture and territory-driven road teardown in `noFlag` / `GameWorld`

### Output
- File: `road_log.pb`
- Format: length-delimited protobuf stream
- Stream layout:
  - one `RoadLogHeader`
  - followed by repeated `RoadLogRecord`

### Notes
- `player_id` values are written as 1-based IDs.
- Demolition events carry reason codes, with optional initiator player IDs when
  known.
- The logger records player-built road construction, split replacement
  segments, implicit building-front access roads, and road/flag teardown events.

## TroopsLimitEventLogger

### Purpose
Tracks changes to the effective troop limit of military buildings.

### Hooks
- Troop-limit recalculation in `nobMilitary`

### Output
- File: `troops_limit_log.csv`
- Format: CSV
- Header:
  - `gameframe,playerId,newLimit,buildingType,buildingId`

## ToolPriorityEventLogger

### Purpose
Tracks AI tool-priority table updates per player.

### Hooks
- Tool-priority adjustments in `AIAdjuster`

### Output
- File: `tool_priority.csv`
- Format: CSV
- Header:
  - `gameframe,playerId,<one column per tool>`
