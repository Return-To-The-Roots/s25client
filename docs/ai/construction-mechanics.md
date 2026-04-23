# AI Construction Job Mechanics

See also:

- [position-finding.md](position-finding.md) — `FindBestPosition` /
  `FindPositionForBuildingAround` are the searches that feed (and can
  trip the cooldown described below).
- [road-route-selection.md](road-route-selection.md) — details for
  `ConnectFlagToRoadSystem`, `BuildAlternativeRoad`, and the road
  scoring referenced by the road/flag utilities here.
- [configuration.md](configuration.md) — `wantedParams`, `disableBuilding`,
  and `bqPenalty` knobs that gate `Wanted()` and route choices.
- [performance-profiling.md](performance-profiling.md) — `ExecuteAIJob`,
  `ExecuteGlobalBuildJobs`, `ExecuteBuildJobs`, and `ExecuteConnectJobs`
  cover this dispatch loop in profiler output.

## Job Queues and Deduplication
- `AddGlobalBuildJob` keeps a single global job per `BuildingType`, so strategic one-off goals
  (e.g., “build a shipyard somewhere”) do not stack and spam the planner.
- Global jobs also participate in a per-`BuildingType` retry cooldown after a full-map search
  fails to find any valid position. The cooldown is only for `SearchMode::Global`; enqueueing
  still happens normally so event-driven goals such as `Storehouse` are not lost.
- `AddBuildJob` handles per-location work. It forbids invalid shipyard spots, allows multiple
  simultaneous military builds, but deduplicates non-military jobs by `type + around` so only
  one farmhouse, quarry, etc. is queued for a specific area at a time. Jobs can be pushed to
  the front of the deque when the caller wants immediate attention.
- `AddConnectFlagJob` ensures each flag enters the connection queue at most once, avoiding
  redundant “hook this node up” tasks.

## Where `buildJobs` Entries Come From
- `AIPlayerJH::AddBuildJob` is the only regular gateway into the deque; it wraps the requested
  `BuildingType`, target point, and search mode into a `BuildJob` and forwards it to
  `AIConstruction::AddBuildJob` (optionally to the front of the queue for urgent orders).
- Strategic planners feed this entry point in multiple ways: direct planner calls, helper loops
  such as `AddBuildJobAroundEveryWarehouse/MilBld`, scripted expansion hooks like
  `AddMilitaryBuildJob`, and Lua automation that either force-places a site or, when `forced`
  is false, pends a normal build job after `Wanted()` confirms demand.
- Existing `BuildJob`s also recycle into the deque. Whenever construction fails (bad BQ, road
  connection impossible, destroyed flag, etc.) or needs to retry with a downgraded building,
  the job reissues itself by calling `aijh.AddBuildJob` with the original parameters.
- Follow-up chains replenish the deque: finishing certain economic buildings (farm → well,
  mill → bakery, pig farm → slaughterhouse, beer producers → well) enqueue downstream jobs so
  the economy stays balanced automatically.
- Event handlers (`HandleBuilingDestroyed`, loss of territory/resources, etc.) and building
  search jobs (`FindPositionForBuildingAround`, `SearchMode::Global`) all funnel through the
  same wrapper, so every AI stimulus still results in the same deduped `buildJobs` entries.

## Execution Order
- `ExecuteJobs(limit)` interleaves work from three queues: up to five connection jobs,
  followed by up to five global build jobs, then location-specific build jobs, all bounded by
  the supplied limit. This keeps AI cycles short while ensuring each queue makes forward
  progress every tick.
- Connect and build jobs that report `JobState::Finished` or `JobState::Failed` drop out;
  everything else is requeued at the back so transient blockers (enemy presence, missing
  wares) retry later.
- Global jobs live in an ordered multiset. When a job cannot run, its priority is decreased
  before being reinserted, preventing one stubborn goal from starving newer entries.
- One exception exists for cooldown-blocked global jobs: if a job reaches execution while its
  per-type global-search cooldown is still active, it skips `FindBestPosition()`, stays queued,
  and is reinserted without losing priority because no expensive search was attempted.

## Global Search Cooldown
- The cooldown starts only when a global building-position search returns
  `MapPoint::Invalid()`. It does not start for later failures such as `SetBuildingSite()`
  rejection, BQ changes after placement, road-connection failure, or destroyed sites/flags.
- The timer is tracked in `AIConstruction` per `BuildingType` and currently lasts `500`
  game frames. The current implementation uses time expiry only; unrelated world updates do
  not clear it.
- This throttle complements the global position cache instead of replacing it. Frequent cache
  invalidations can still happen, but the cooldown prevents repeated full-map rescans for the
  same building type during that short backoff window.

## Construction Reservation Tracking
- `constructionlocations` collects every point touched by orders during the current navigation
  wavefront. `ConstructionOrdered` pushes the build site, and helpers such as
  `SetFlagsAlongRoad` and successful road orders add intermediate tiles.
- `CanStillConstructHere` checks the list and refuses any new job whose target is within 12
  tiles of an active site, spacing projects out so carriers do not pile into the same area.
- `constructionorders` mirrors `BuildingType` values; once `ConstructionsExecuted` is called
  the counts reset. `Wanted()` compares these counters to the `BuildingPlanner` demand so the
  AI only spawns as many work orders as strategic planning requested.

## Road and Flag Utilities
- `FindFlags` gathers nearby valid `noFlag` objects for the player (wrapping-safe) so the AI
  knows where existing infrastructure lies.
- `ConnectFlagToRoadSystem` searches those candidates, rejecting options that lack a path,
  exceed two non-flaggable tiles in a row, or already link back to the origin. The winning
  route is fed into `MinorRoadImprovements/BuildRoad`, which issues the build order and
  records both end flags in `constructionlocations` so future commands avoid the corridor.
- `BuildAlternativeRoad` compares a proposed shortcut with the current path to the same flag
  and only commits if it substantially shrinks the travel length. `IsConnectedToRoadSystem`
  and `FindTargetStoreHouseFlag` provide quick checks to see whether a flag is already tied
  into the network via the closest warehouse.

## Choosing What to Build
- `GetSmallestAllowedMilBuilding` / `GetBiggestAllowedMilBuilding` scan capability tables so
  later decisions know which blueprints are legal at the moment.
- `ChooseMilitaryBuilding` factors in garrison counts, stone supply, harbor pressure, nearby
  enemies, and a bit of randomness to break ties. Special cases allow catapult pushes or
  fallback to guardhouses when expansion space is tight.
- `Wanted()` gates every non-road job: it consults `BuildingPlanner` demand, current
  inventory (e.g., sawmills need sufficient wood), and rejects forbidden options such as
  catapults when the technology is locked.
- Helper counts like `CountUsualBuildingInRadius`, `OtherUsualBuildingInRadius`, and
  `OtherStoreInRadius` ensure the planner does not seed multiple identical buildings on top
  of each other before previous jobs finish.
