# AI Road Route Selection

The AI does not run a global road-network optimizer. Road construction is a
local heuristic layered on top of two lower-level pathfinders:

- `AIConstruction::ConnectFlagToRoadSytem()` chooses which nearby existing flag
  to connect to.
- `AIQueryService::FindFreePathForNewRoad()` finds a road-buildable free-terrain
  route for the new segment.
- `AIQueryService::FindPathOnRoads()` measures the existing road-network
  distance from a candidate flag back to the chosen warehouse.

This document describes how those pieces combine when the AI decides what it
considers the "best" road.

## Trigger Flow

- New building jobs check whether the building flag is already connected to the
  road system.
- If not, `BuildJob::ExecuteJob()` calls
  `AIConstruction::ConnectFlagToRoadSytem()`.
- For non-military buildings, `BuildJob::TryToBuildSecondaryRoad()` may later
  call `BuildAlternativeRoad()` to add a shortcut.

## What the AI Optimizes For

The main connection heuristic is not trying to optimize travel time over the
entire transport graph. It is trying to find a nearby owned flag such that:

- the newly built road segment is short,
- the chosen endpoint already reaches a warehouse by road,
- the proposed route leaves room for flags,
- the route avoids long stretches of non-flaggable nodes.

The warehouse target is chosen by straight map distance in
`FindTargetStoreHouseFlag()`, not by current road distance and not by carrier
traffic.

## Candidate Flag Search

`AIConstruction::FindFlags()` gathers candidate endpoints around the source
flag:

- only the AI player's own flags are eligible,
- the search is bounded by a radius (`14` by default for the main connection,
  `10` for alternative roads),
- the result list is capped at `30` flags via `GetPointsInRadius<30>()`.

`GetPointsInRadius()` walks outward ring by ring around the source point in a
fixed order. As a result:

- closer flags are typically seen first,
- dense areas may hit the `30`-flag cap before all plausible endpoints are
  examined,
- the AI therefore picks the best route among the scanned candidates, not among
  every possible flag on the map.

## Building the New Road Segment

For each candidate flag, `ConnectFlagToRoadSytem()` asks
`FindFreePathForNewRoad()` for a free-terrain route from the source flag to the
candidate.

That query uses `FreePathFinder::FindPathAlternatingConditions()`, which is an
A*-style search with a maximum length of `100`.

The pathfinder enforces several road-specific constraints:

- every traversed point must be inside player territory,
- every traversed point must be road-buildable according to
  `GameWorldBase::IsRoadAvailable()`,
- on alternating steps, the node must also be flaggable for land roads,
- even-step nodes that are too close to previously chosen even-step nodes are
  rejected.

This alternating-node rule is what makes the produced route compatible with
regular flag placement along the road.

`GameWorldBase::IsRoadAvailable()` rejects nodes that:

- contain blocking objects,
- lie on border stones,
- already carry roads,
- touch prohibited surroundings such as charburner piles,
- have unsuitable terrain.

For land roads, at least one surrounding terrain slot must support something
better than `TerrainBQ::Nothing`, otherwise the point is not considered a valid
road tile.

## Main Connection Scoring

After a free-terrain route is found, `ConnectFlagToRoadSytem()` filters and
scores it.

First it rejects candidates that:

- produce more than `2` consecutive non-flaggable points,
- do not already have a road path to the chosen warehouse flag,
- are already connected back to the source flag through the current road
  network.

Then it computes:

```text
score = oddPenalty + 2 * newRoadLength + warehouseRoadDistance
        + 10 * maxNonFlaggableRun
        + bqPenalty.roadRoute * routeBQPenalty
```

Where:

- `oddPenalty` is `5` when the new road length is odd, otherwise `0`,
- `newRoadLength` is the number of steps in the newly built segment,
- `warehouseRoadDistance` is the existing road-network distance from the
  candidate flag to the selected warehouse,
- `maxNonFlaggableRun` is the longest consecutive stretch of
  `BuildingQuality::Nothing` along the new route,
- `routeBQPenalty` is the summed downgrade cost caused by the hypothetical road
  reducing building quality on route tiles and their nearby affected plots,
- `bqPenalty.roadRoute` comes from `AIConfig` and defaults to `1.0`.

The doubled weight on `newRoadLength` means the AI prefers short new
construction even when the total end-to-end route would be similar either way,
while the BQ term lightly discourages routes that destroy more valuable
building plots.

## How Existing Road Distance Is Measured

`warehouseRoadDistance` comes from `AIQueryService::FindPathOnRoads()`, which
calls `RoadPathFinder::FindPath(..., wareMode = false)`.

This matters because `wareMode = false` means:

- road cost is just the sum of `RoadSegment::GetLength()`,
- water roads are excluded,
- congestion penalties are ignored,
- carrier availability penalties are ignored.

So for road-building decisions the AI measures plain land-road length, not
estimated delivery time.

The road pathfinder itself is another A* search over `noRoadNode` graph nodes
(flags and certain buildings), using:

- `cost = currentCost + roadSegmentLength`,
- `estimate = cost + mapDistanceToGoal`.

It therefore prefers the shortest existing land-road path between two road
nodes.

## Secondary Roads

After a building is connected, non-military jobs may attempt a second road via
`BuildAlternativeRoad()`.

This pass:

- searches nearby owned flags again,
- requires the candidate to already be connected to the road system,
- computes a new free-terrain road to that flag,
- compares that new segment against the current road-network distance.

The road is built only if:

- no current path exists, or
- `newLength * 5 + bqPenalty.roadRoute * routeBQPenalty < oldLength`.

So secondary roads are conservative shortcuts. They are not meant to fine-tune
every route, only to add clearly superior bypasses.

## Important Non-Obvious Details

- `FindTargetStoreHouseFlag()` uses nearest warehouse by straight map distance,
  not by road distance.
- `FindFlags()` is capped to `30` results, so dense local flag clusters can hide
  farther-but-better options.
- `MinorRoadImprovements()` currently returns immediately into `BuildRoad()`.
  The code below that early return is effectively disabled.
- The construction heuristic is local and greedy. It does not restructure the
  network globally or reason about future traffic beyond the limited
  alternative-road pass.

## Practical Takeaways

- The AI usually prefers connecting to a nearby flag that already lies on a
  short route to the nearest warehouse.
- It avoids routes that create long non-flaggable stretches, because those are
  fragile for flag placement and logistics.
- It does not account for live carrier congestion when selecting the endpoint
  for a new road.
- If a route choice looks odd in-game, likely causes are:
  - a better flag was outside the search radius,
  - a better flag was beyond the `30`-candidate cap,
  - the nearest warehouse by map distance was not the best warehouse by road,
  - the free-terrain road pathfinder rejected the more obvious route due to
    flag-placement constraints.
