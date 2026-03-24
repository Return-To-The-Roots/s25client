# S2 AI Overview

For the JH AI directory split and file-placement rules, see
`libs/s25main/ai/aijh/README.md`.

## Player-Level Architecture
- `libs/s25main/ai/AIPlayer.h` defines the abstract façade every bot shares: it tracks the owning `GamePlayer`, queues `GameCommand` objects, and exposes lifecycle hooks (`RunGF`, `OnChatMessage`).  
- `libs/s25main/ai/aijh/runtime/AIPlayerJH.*` implements the production AI. It wires in module singletons (construction, planner, jobs) and orchestrates frame updates, configuration, and defeat checks. `DummyAI.h` remains a minimal placeholder for testing.

## Command & Event Flow
- `AIQueryService` (`libs/s25main/ai/AIQueryService.*`) owns read-only access to world state, building registers, inventories, visibility, and path-search helpers. `AICommandSink` (`libs/s25main/ai/AICommandSink.*`) owns command emission and queued AI chat/game-command output.  
- `AIInterface` (`libs/s25main/ai/AIInterface.*`) is now a transitional combined facade on top of those two roles. New code should prefer `Queries()` and `Commands()` directly; the remaining wrapper methods exist only for legacy callers that have not been migrated yet.  
- Runtime stimuli are buffered in `AIEventManager`/`AIEvents` (`libs/s25main/ai/AIEventManager.*`, `AIEvents.h`). Events such as `BuildingDestroyed` or `RoadConstructionFailed` are wrapped in lightweight structs and processed by job handlers inside `AIPlayerJH`.

## Planning Components
- `AIConstruction` (`libs/s25main/ai/aijh/planning/AIConstruction.*`) holds build and connection queues (`BuildJob`, `ConnectJob`, `EventJob`, `SearchJob` from `Jobs.*`), schedules execution, and deals with road layout and duplicate suppression.  
- `BuildingPlanner` tracks stockpiles, active constructions, and desired building counts. It feeds `AIConstruction` with priorities and expansion hints.  
- `AIMap` and `Node` (`libs/s25main/ai/aijh/runtime/AIMap.h`) cache per-hex metadata such as ownership, building quality, failed placements, and reachability to limit expensive world lookups.

## Build Job Planning & Execution
- **Job creation**: `AIPlayerJH` spawns jobs through helpers like `AddBuildJob`, `AddGlobalBuildJob`, and `AddBuildJobAroundEveryWarehouse` (`libs/s25main/ai/aijh/runtime/AIPlayerJH.cpp`). Calls originate from planner passes (`PlanNewBuildings`), event responses (e.g., rebuilding on `BuildingDestroyed`), and chain reactions inside `BuildJob::BuildMainRoad` that enqueue complementary buildings (wells, bakeries, etc.). `BuildingPlanner::Wanted` gates every request so surplus work is discarded early.
- **Queue topology**: `AIConstruction::AddBuildJob` deduplicates non-military jobs per location and keeps military orders in arrival order. A separate `globalBuildJobs` multiset re-queues deferred work with a decreasing `priority`, `milBuildJobs` favors front-line expansion, and `connectJobs` holds delayed road connections. `ConstructionOrdered` records active sites and `constructionlocations`, which `CanStillConstructHere` consults to throttle overlapping orders within ~12 tiles during the same network frame.
- **Execution loop**: Each AI tick invokes `AIConstruction::ExecuteJobs(limit)` to iterate `connectJobs`, `globalBuildJobs`, then the local `buildJobs` deque until the frame budget is exhausted. Jobs that neither finish nor fail are rotated to the back (or reinserted with reduced priority) so stalled work retries later without blocking fresh tasks. Completed or aborted jobs exit quietly; `ConstructionsExecuted` is triggered on network-frame updates to clear throttling state.
- **Build job state machine**: `BuildJob::ExecuteJob` advances through `Start → ExecutingRoad1 → ExecutingRoad2 → ExecutingRoad2_2` (see `libs/s25main/ai/aijh/planning/Jobs.cpp`). `TryToBuild` enforces the 40-site cap, validates demand, picks a placement via `AIPlayerJH::FindBestPosition` (global search) or `FindPositionForBuildingAround` (radius search), and issues `AICommandSink::SetBuildingSite` through the AI facade. Failure to locate a tile or place a site transitions to `Failed`, often re-enqueuing an adjusted job (upgrade/downgrade logic for military buildings, or a requeue when `failed_penalty` is set). `BuildMainRoad` validates the site, asks `AIConstruction::ConnectFlagToRoadSystem` to link it, reacts to domain logic (triggering wells, geologists, military upgrades), and updates AI map state. Non-military buildings progress to `TryToBuildSecondaryRoad`, which attempts a second connection via `BuildAlternativeRoad`; success leads to `ExecutingRoad2_2`, where `AIPlayerJH::RecalcGround` finalizes terrain updates.
- **Failure recovery**: When road construction fails or the node becomes unreachable, the AI command side tears down the provisional site and flag, marks `AINode::reachable` false, and requeues either a global or local job. `constructionorders` counts pending builds per type so planners can back off aggressively flagged categories, preventing infinite loops if terrain or resources render a placement impossible.

## Resource & Position Evaluation
- `AIResourceMap` (`libs/s25main/ai/aijh/runtime/AIResourceMap.*`) maintains per-resource heatmaps. It switches between diminishing and replenishable update strategies and can block tiles after they are reserved.  
- `GlobalPositionFinder` (`libs/s25main/ai/aijh/planning/GlobalPositionFinder.*`) combines resource maps, planner statistics, and proximity rules to rank candidate spots using `RatedPoint`/`RatedPointSet` (`libs/s25main/ai/RatedPoint.h`). Specialized handlers exist for woodcutters, foresters, and farms; the fallback delegates to `AIPlayerJH::SimpleFindPosition`.

## Configuration & Tuning
- YAML-driven weights live in `ai/aijh/config/AIConfig.*`, `ai/aijh/config/WeightParams.*`, `ai/aijh/config/WeightParser.*`, and `ai/aijh/debug/StatsConfig.*`. `initAIConfig` loads defaults, applies planner- and position-finder overrides, then exposes `AI_CONFIG` for runtime queries.  
- Build demand, proximity minima, and productivity curves are described via `BuildParams` and `WantedParams`, enabling balance tweaks without recompilation. Sanitizing changes requires re-running the AI or reloading the save to rebuild caches.

## Extensibility Notes
- To add new behavior, implement a subclass of `AIJob` (`Jobs.h`) and enqueue it via `AIPlayerJH::ExecuteAIJob`.  
- When introducing new event types, extend `AIEvents::EventType`, provide a concrete payload struct, and branch in `AIPlayerJH::Handle*` helpers so planners can react consistently.  
- Keep `AICommandSink` as the only writer of `GameCommand` buffers to preserve validation/injection points shared with the networking layer. `AIInterface` may still forward to it for legacy call sites, but new code should not introduce fresh write helpers on the combined facade.
