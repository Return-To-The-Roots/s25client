# S2 AI Overview

## Player-Level Architecture
- `libs/s25main/ai/AIPlayer.h` defines the abstract façade every bot shares: it tracks the owning `GamePlayer`, queues `GameCommand` objects, and exposes lifecycle hooks (`RunGF`, `OnChatMessage`).  
- `libs/s25main/ai/aijh/AIPlayerJH.*` implements the production AI. It wires in module singletons (construction, planner, jobs) and orchestrates frame updates, configuration, and defeat checks. `DummyAI.h` remains a minimal placeholder for testing.

## Command & Event Flow
- `AIInterface` (`libs/s25main/ai/AIInterface.*`) sits between the AI logic and the world. It inherits `GameCommandFactory`, so calls like `BuildRoad` or `OrderBuilding` are converted to queued commands consumed by the engine. The interface also exposes read-only helpers for territory, visibility, inventories, and path searches.  
- Runtime stimuli are buffered in `AIEventManager`/`AIEvents` (`libs/s25main/ai/AIEventManager.*`, `AIEvents.h`). Events such as `BuildingDestroyed` or `RoadConstructionFailed` are wrapped in lightweight structs and processed by job handlers inside `AIPlayerJH`.

## Planning Components
- `AIConstruction` (`libs/s25main/ai/aijh/AIConstruction.*`) holds build and connection queues (`BuildJob`, `ConnectJob`, `EventJob`, `SearchJob` from `Jobs.*`), schedules execution, and deals with road layout and duplicate suppression.  
- `BuildingPlanner` tracks stockpiles, active constructions, and desired building counts. It feeds `AIConstruction` with priorities and expansion hints.  
- `AIMap` and `Node` (`libs/s25main/ai/aijh/AIMap.h`) cache per-hex metadata such as ownership, building quality, failed placements, and reachability to limit expensive world lookups.

## Build Job Planning & Execution
- **Job creation**: `AIPlayerJH` spawns jobs through helpers like `AddBuildJob`, `AddGlobalBuildJob`, and `AddBuildJobAroundEveryWarehouse` (`libs/s25main/ai/aijh/AIPlayerJH.cpp`). Calls originate from planner passes (`PlanNewBuildings`), event responses (e.g., rebuilding on `BuildingDestroyed`), and chain reactions inside `BuildJob::BuildMainRoad` that enqueue complementary buildings (wells, bakeries, etc.). `BuildingPlanner::Wanted` gates every request so surplus work is discarded early.
- **Queue topology**: `AIConstruction::AddBuildJob` deduplicates non-military jobs per location and keeps military orders in arrival order. A separate `globalBuildJobs` multiset re-queues deferred work with a decreasing `priority`, `milBuildJobs` favors front-line expansion, and `connectJobs` holds delayed road connections. `ConstructionOrdered` records active sites and `constructionlocations`, which `CanStillConstructHere` consults to throttle overlapping orders within ~12 tiles during the same network frame.
- **Execution loop**: Each AI tick invokes `AIConstruction::ExecuteJobs(limit)` to iterate `connectJobs`, `globalBuildJobs`, then the local `buildJobs` deque until the frame budget is exhausted. Jobs that neither finish nor fail are rotated to the back (or reinserted with reduced priority) so stalled work retries later without blocking fresh tasks. Completed or aborted jobs exit quietly; `ConstructionsExecuted` is triggered on network-frame updates to clear throttling state.
- **Build job state machine**: `BuildJob::ExecuteJob` advances through `Start → ExecutingRoad1 → ExecutingRoad2 → ExecutingRoad2_2` (see `libs/s25main/ai/aijh/Jobs.cpp`). `TryToBuild` enforces the 40-site cap, validates demand, picks a placement via `AIPlayerJH::FindBestPosition` (global search) or `FindPositionForBuildingAround` (radius search), and issues `AIInterface::SetBuildingSite`. Failure to locate a tile or place a site transitions to `Failed`, often re-enqueuing an adjusted job (upgrade/downgrade logic for military buildings, or a requeue when `failed_penalty` is set). `BuildMainRoad` validates the site, asks `AIConstruction::ConnectFlagToRoadSystem` to link it, reacts to domain logic (triggering wells, geologists, military upgrades), and updates AI map state. Non-military buildings progress to `TryToBuildSecondaryRoad`, which attempts a second connection via `BuildAlternativeRoad`; success leads to `ExecutingRoad2_2`, where `AIPlayerJH::RecalcGround` finalizes terrain updates.
- **Failure recovery**: When road construction fails or the node becomes unreachable, `AIInterface` tears down the provisional site and flag, marks `AINode::reachable` false, and requeues either a global or local job. `constructionorders` counts pending builds per type so planners can back off aggressively flagged categories, preventing infinite loops if terrain or resources render a placement impossible.

## Resource & Position Evaluation
- `AIResourceMap` (`libs/s25main/ai/aijh/AIResourceMap.*`) maintains per-resource heatmaps. It switches between diminishing and replenishable update strategies and can block tiles after they are reserved.  
- `PositionFinder` (`libs/s25main/ai/aijh/PositionFinder.*`) combines resource maps, planner statistics, and proximity rules to rank candidate spots using `RatedPoint`/`RatedPointSet` (`libs/s25main/ai/RatedPoint.h`). Specialized handlers exist for woodcutters, foresters, and farms; the fallback delegates to `AIPlayerJH::SimpleFindPosition`.

## Configuration & Tuning
- YAML-driven weights live in `AIConfig.*`, `WeightParams.*`, `WeightParser.*`, and `StatsConfig.*`. `initAIConfig` loads defaults, applies planner- and position-finder overrides, then exposes `AI_CONFIG` for runtime queries.  
- Build demand, proximity minima, and productivity curves are described via `BuildParams` and `WantedParams`, enabling balance tweaks without recompilation. Sanitizing changes requires re-running the AI or reloading the save to rebuild caches.

## Extensibility Notes
- To add new behavior, implement a subclass of `AIJob` (`Jobs.h`) and enqueue it via `AIPlayerJH::ExecuteAIJob`.  
- When introducing new event types, extend `AIEvents::EventType`, provide a concrete payload struct, and branch in `AIPlayerJH::Handle*` helpers so planners can react consistently.  
- Keep `AIInterface` as the only writer of `GameCommand` buffers to preserve validation/injection points shared with the networking layer.
