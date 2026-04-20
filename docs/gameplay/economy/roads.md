# Roads And AI Road Building

This note summarizes how roads work in the runtime economy and how AI players
decide to build them. It is based on the current `s25main` implementation,
mainly `GameWorld`, `RoadSegment`, `noFlag`, `noRoadNode`, `GamePlayer`, and
the AI construction code in `ai/aijh`.

Related document:

- `docs/ai/road-route-selection.md` focuses on the scoring details of the AI's
  route choice. This document covers the broader gameplay mechanics first, then
  shows how the AI plugs into them.

## Core Model

The road network is a graph of `noRoadNode` endpoints connected by
`RoadSegment` edges.

- `noRoadNode` is the common base for flags and buildings.
- Player-built roads connect one flag to another flag.
- Every building also owns a built-in one-step road from its front flag to the
  building door.

That building entry road is created in `noBaseBuilding`:

- the flag in front of the building is created if needed,
- a one-step `RoadSegment` with route `{NorthWest}` is attached between the
  flag and the building,
- this segment is not treated like a normal inter-flag road in the player's
  registered road list.

In practice this means the logistics graph is:

- warehouse/building <-> front flag as a fixed short segment,
- front flag <-> front flag through player-built road segments.

## What A Built Road Does

`GameWorld::BuildRoad()` is the world-side validator and constructor for road
commands.

It requires:

- a valid start flag owned by the player,
- a route with at least two steps,
- every intermediate node to stay inside player territory and satisfy
  `IsRoadAvailable()`,
- the end point to be either an owned flag or a point where a new flag may be
  placed.

If the end point has no flag yet, the world places one automatically before the
segment is created.

Once validated, the world:

- writes road tiles onto the map,
- recalculates building quality around the touched tiles,
- creates one `RoadSegment`,
- attaches that segment to both endpoint flags,
- notifies the owning `GamePlayer` via `NewRoadConnection()`,
- publishes a `RoadNote`.

For human players, the `AUTOFLAGS` addon can also place extra flags along a
land road immediately after construction. AI players do not use that addon
path.

## Placement And Invalidity Rules

`GameWorldBase::IsRoadAvailable()` rejects road tiles when any of the
following is true:

- the node contains a blocking object,
- the node contains a border stone,
- any road already touches that point,
- a neighboring node has `BlockingManner::NothingAround` such as a charburner
  pile,
- terrain is unsuitable.

For land roads, at least one surrounding terrain slot must still support some
building quality above `TerrainBQ::Nothing`. Water roads instead require a
water point.

Flags also have spacing constraints:

- `SetFlag()` refuses points whose building quality is `Nothing`,
- `SetFlag()` refuses points that have another flag in a neighboring node,
- `BuildRoad()` also refuses an endpoint if a flag cannot be placed there.

Roads are not permanent. They are torn down when their endpoints or territory
become invalid:

- destroying a flag or building destroys attached road segments,
- splitting a road by placing a flag replaces one segment with two,
- territory recalculation destroys roads that now cross border or foreign land.

When a road is destroyed, wares recalculate routes and figures on that segment
lose their workplace and start wandering until reassigned.

## Flags, Wares, And Carriers

Flags are the buffering points of the land economy.

- A flag can store up to 8 wares.
- Each ware at a flag remembers its next desired road direction.
- `noFlag::SelectWare()` chooses the next ware for a segment by the player's
  transport priority table.

`RoadSegment` is the unit of carrier work.

- A normal land segment can have one helper carrier.
- A donkey road can have that helper plus one donkey.
- A water road uses a boat carrier instead of a land helper.

`GamePlayer::FindCarrierForRoad()` assigns a carrier by searching for the
nearest reachable warehouse that can supply the required worker, and for water
roads also a boat.

The pathfinder used for ware movement is road-aware:

- `GameWorld::FindPathForWareOnRoads()` calls `RoadPathFinder` in ware mode,
- `noFlag::GetPunishmentPoints()` adds penalties for queued wares, missing
  carriers, or carriers still on the way,
- this lets wares prefer less congested segments when alternatives exist.

Human pathfinding on roads uses the same graph but without those ware-traffic
penalties.

## Donkey Roads

Roads do not start as donkey roads. A segment upgrades when its normal carrier
proves busy enough.

- `nofCarrier` measures productivity over `6000` game frames.
- If a normal carrier reaches `80%` productivity, it calls
  `RoadSegment::UpgradeDonkeyRoad()`.
- The segment changes from `Normal` to `Donkey`, the map road tiles are
  updated, and both endpoint flags are upgraded visually.
- `RoadSegment::TryGetDonkey()` then asks the owning player to order a
  `PackDonkey` worker from the nearest reachable warehouse.

So donkey roads are an automatic consequence of sustained traffic, not
something the AI currently plans explicitly.

## How The AI Builds Roads

The AI issues the same game commands as a human player. It does not bypass the
world rules.

`BuildJob::ExecuteJob()` drives building construction in phases:

1. `TryToBuild()` chooses a site and issues `SetBuildingSite`.
2. `BuildMainRoad()` checks whether the building's front flag is already
   connected to the road system.
3. If not connected, it calls `AIConstruction::ConnectFlagToRoadSytem()`.
4. For non-military buildings, `TryToBuildSecondaryRoad()` may then call
   `BuildAlternativeRoad()` to add a shortcut.

Military buildings stop after the first connection. Non-military buildings may
get a second road if a clearly better nearby shortcut exists.

## How The AI Chooses A Main Road

The AI is local and greedy here. It does not run a global road optimizer.

`AIConstruction::ConnectFlagToRoadSytem()` does this:

1. Pick a target warehouse flag with `FindTargetStoreHouseFlag()`.
   This is the nearest warehouse by straight map distance, not by road
   distance.
2. Collect nearby owned flags with `FindFlags()`.
   This search is radius-limited and capped at 30 candidates.
3. For each candidate, ask `AIQueryService::FindFreePathForNewRoad()` for a
   free-terrain route from the source flag to that candidate.
4. Reject candidates whose proposed route:
   - cannot be built under the world road rules,
   - contains more than 2 consecutive non-flaggable points,
   - does not already have a road path from the candidate to the chosen
     warehouse,
   - is already connected back to the source through the existing road network.
5. Score the remaining candidates and keep the best one.

The free-terrain route search uses
`FreePathFinder::FindPathAlternatingConditions()`.

For land roads this enforces:

- all traversed nodes stay in owned territory,
- all traversed nodes satisfy `IsRoadAvailable()`,
- alternating steps must also stay flaggable,
- even-step flag positions on the planned route may not bunch too closely.

That alternating-step rule is why the AI strongly prefers routes where regular
flag placement remains possible.

The exact scoring formula is documented in
`docs/ai/road-route-selection.md`. At a high level, the AI prefers:

- short new road segments,
- candidate flags that already sit on a short route to a warehouse,
- routes with few long non-flaggable stretches,
- routes that destroy less surrounding building quality.

Once a route is chosen, `AIConstruction::BuildRoad()` sends a normal
`BuildRoad` command through the AI command sink.

## AI Follow-Up After Road Construction

The AI listens to `RoadNote` notifications through `AIPlayerJH`.

- On success, `AIRoadController::HandleRoadConstructionComplete()` may place
  additional intermediate flags along long segments with
  `AIConstruction::SetFlagsAlongRoad()`.
- On failure, `AIRoadController::HandleRoadConstructionFailed()` removes any
  now-useless partial branch and requeues a reconnect attempt.
- The AI also scans for orphaned building sites and can enqueue `ConnectJob`s
  to reconnect them later.

This matters because AI road building is asynchronous:

- the AI enqueues build commands,
- the world accepts or rejects them later,
- follow-up cleanup and extra flag placement happen from the resulting road
  events.

## How The AI Handles `ConnectJob`s

`ConnectJob`s are the AI's deferred "make sure this flag really ends up on the
road network" work items.

They sit in their own queue inside `AIConstruction`:

- `AddConnectFlagJob()` deduplicates by flag position, so one stuck building
  site does not flood the planner with identical reconnect work.
- `ExecuteJobs()` processes connect jobs before global or local build jobs,
  but only up to `5` connection jobs per pass.
- Jobs that neither finish nor fail are pushed back to the queue and retried
  later.

The AI enqueues them from several repair-style situations:

- when a colony is founded and its new flag still needs a land connection,
- when military logistics wants a frontier military building linked up,
- when a road attempt failed and cleanup removed a dead-end fragment,
- when periodic scans, run only if no build or connect backlog exists, find
  building sites whose front flag still has no usable road.

`ConnectJob::ExecuteJob()` is deliberately conservative:

1. If another construction order was just issued nearby, the job does nothing
   for now and stays queued. This avoids overlapping road orders in the same
   area.
2. If the flag no longer exists, the job fails and is discarded.
3. If the flag belongs to a military building that already has any extra road
   edge, the job finishes immediately. This prevents military sites from
   collecting multiple side connections.
4. If the flag is still disconnected, the job calls
   `ConnectFlagToRoadSytem(flag, route, 24)` to try one normal road-building
   attempt.
5. If that call cannot find a valid route, the job fails. If it does issue a
   road order, the job stays queued until a later tick sees the flag as truly
   connected.
6. Once the flag reaches the road system, the job finishes and
   `RecalcGround()` updates the local AI terrain/building-quality view for the
   finished route.

So `ConnectJob`s are less about planning new expansion roads and more about
finishing, retrying, or repairing local connections after the main build logic
or runtime events left a flag temporarily isolated.

## Secondary Roads And Cleanup

For non-military buildings, `BuildAlternativeRoad()` can add one more nearby
connection after the main road succeeds.

It:

- searches nearby owned flags again,
- requires the candidate to already belong to the road system,
- computes a new free-terrain route,
- compares that new segment against the existing road distance,
- only builds if the shortcut is clearly worthwhile.

The AI also has road-pruning logic in `AIRoadController`:

- failed road attempts can trigger removal of dead-end fragments,
- exhausted buildings can cause nearby unused roads to be removed,
- unconnected sites can be requeued for later road construction.

## Practical Summary

- The economic road network is flag-to-flag; building access is a separate
  built-in one-step segment.
- Flags are the real logistics buffers, and road segments are the units of
  carrier assignment.
- Land-road placement is constrained by ownership, terrain, border stones,
  nearby flags, and current roads.
- Busy segments can upgrade themselves into donkey roads automatically.
- AI players build roads through the same `BuildRoad` command path as humans.
- The AI chooses local connections to nearby owned flags, not globally optimal
  transport networks.
- Non-military AI buildings may get a second shortcut road; military buildings
  usually stop at one connection.
