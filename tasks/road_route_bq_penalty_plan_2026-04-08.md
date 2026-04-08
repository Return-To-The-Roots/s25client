# AI Road Candidate BQ Penalty Plan

## Goal

Add a light route-score penalty for candidate AI road connections that reduce
building quality on the road tiles themselves or on nearby plots affected by
the new road.

This is a planning document only. No code changes are included here.

## Motivation

The current AI road candidate score in
`libs/s25main/ai/aijh/planning/AIConstruction.cpp` is:

```text
score = oddPenalty + 2 * newRoadLength + warehouseRoadDistance
        + 10 * maxNonFlaggableRun
```

This can prefer a candidate that is path-valid and logistically short even when
the new road damages local building potential more than another slightly longer
candidate. In practice that shows up as the AI connecting to a visually less
attractive flag because the current score does not account for lost building
space.

## Current Relevant Mechanics

### Where the score is computed

- Main road candidates are scored in
  `AIConstruction::ConnectFlagToRoadSytem()`.
- Secondary shortcut candidates are evaluated in
  `AIConstruction::BuildAlternativeRoad()`.

### How road placement affects building quality

The world updates building quality incrementally when roads are built:

- `GameWorld::BuildRoad()` calls `RecalcBQForRoad(end)` for every road point on
  the route.
- `GameWorldBase::RecalcBQForRoad(pt)` recalculates:
  - `pt`
  - `East(pt)`
  - `SouthEast(pt)`
  - `SouthWest(pt)`

That gives a concrete first approximation for the "plots affected by this road
step" footprint.

### Why nearby plots matter

`BQCalculator` reduces or blocks BQ not only on the road tile itself, but also
through neighboring-road checks:

- a road on the point downgrades it to `Flag`,
- a road on attachment points can downgrade nearby `Castle` plots to `House`,
- the house-flag feasibility check can also change the final BQ result.

So the penalty should not only look at route tiles. It should evaluate the same
local footprint that real road construction recalculates.

## Recommended Design

### 1. Add a dedicated route BQ penalty helper

Introduce a read-only helper that estimates the BQ damage of a hypothetical
road route before issuing any command.

Recommended placement:

- `AIQueryService`, because the logic is read-only and depends on world/BQ
  queries.

Suggested API shape:

```cpp
unsigned EstimateRoadRouteBQPenalty(MapPoint start, const std::vector<Direction>& route) const;
```

Optional internal helpers:

- collect affected plots for a hypothetical route,
- compute hypothetical post-road BQ for a plot,
- map a BQ decrease to penalty points.

### 2. Simulate hypothetical road occupancy without mutating the world

Do not modify the world or AI map state during scoring.

Recommended strategy:

- Build a set of hypothetical road points from `start + route`.
- Reuse `BQCalculator` with a custom `isOnRoad(pt)` lambda:
  - `true` if `gwb.IsOnRoad(pt)` already,
  - `true` if `pt` is one of the hypothetical road points,
  - otherwise `false`.

This stays close to the real BQ calculation logic instead of recreating BQ
rules manually in the AI.

## Affected Plot Footprint

### Recommended first implementation

Match the world's real incremental update footprint:

For each road point `pt` on the planned route, include:

- `pt`
- `East(pt)`
- `SouthEast(pt)`
- `SouthWest(pt)`

Use a unique set so overlapping footprints are only counted once.

Why this is the right starting point:

- it matches `RecalcBQForRoad()`,
- it is small and cheap,
- it is grounded in the actual world update logic,
- it avoids guessing an arbitrary radius.

### Optional later expansion

If playtesting shows missed cases, expand the footprint only after verifying
that the world-side BQ impact extends beyond the current recalculation set.

## Converting BQ Damage Into Score Penalty

### Do not use raw enum subtraction

`BuildingQuality` enum order is not a simple severity ladder because `Mine` is
special:

- `Nothing`
- `Flag`
- `Mine`
- `Hut`
- `House`
- `Castle`
- `Harbor`

So the downgrade penalty should use an explicit mapping, not `old - new`.

### Recommended severity mapping

Use an explicit "build potential value" table and charge only positive drops.

Suggested starting mapping:

- `Nothing = 0`
- `Flag = 1`
- `Hut = 2`
- `House = 3`
- `Castle = 4`
- `Harbor = 4`
- `Mine = 3`

Then for each affected plot:

```text
delta = max(0, value(beforeBQ) - value(afterBQ))
```

And:

```text
routeBQPenalty = sum(delta for all affected plots)
```

This gives useful behavior immediately:

- `Castle -> Flag` costs `3`
- `House -> Nothing` costs `3`
- `Flag -> Nothing` costs `1`
- unchanged or improved plots cost `0`

### Final score integration

Add a light multiplier to the existing route score:

```text
score = oldScore + BQ_PENALTY_WEIGHT * routeBQPenalty
```

Recommended starting value:

- `BQ_PENALTY_WEIGHT = 1`

This is intentionally conservative relative to the existing terms:

- `2 * newRoadLength`
- `warehouseRoadDistance`
- `10 * maxNonFlaggableRun`

If the AI still cuts through valuable building ground too often, raise the
weight after testing.

## Where to Apply the New Penalty

### Main road connection

Apply in `AIConstruction::ConnectFlagToRoadSytem()` after `tmpRoute` has been
validated and before the candidate is compared against `shortestLength`.

Recommended change:

- compute `bqPenalty = aii.Queries().EstimateRoadRouteBQPenalty(flag->GetPos(), tmpRoute)`
  or the equivalent compatibility call,
- add it into the candidate score.

### Alternative roads

Also apply in `AIConstruction::BuildAlternativeRoad()`.

Current alternative-road logic compares:

- `newLength * lengthFactor`
- `oldLength`

Recommended adjustment:

- fold the BQ penalty into the "new route cost" side only,
- keep the current shortcut philosophy intact.

Example:

```text
effectiveNewCost = newLength * lengthFactor + BQ_PENALTY_WEIGHT * routeBQPenalty
build if !pathAvailable || effectiveNewCost < oldLength
```

That keeps alternative roads conservative and prevents "shortcut" roads that
destroy too much future building space.

## Implementation Steps

### Phase 1: Add read-only scoring support

Files:

- `libs/s25main/ai/AIQueryService.h`
- `libs/s25main/ai/AIQueryService.cpp`
- possibly a small private helper near the BQ route scoring logic

Actions:

- add a route-BQ-penalty query API,
- collect hypothetical road points from `start + route`,
- collect unique affected plots using the `RecalcBQForRoad()` footprint,
- compute `beforeBQ` from the current world,
- compute `afterBQ` using `BQCalculator` with hypothetical road occupancy,
- sum explicit downgrade penalties.

Output:

- one read-only function that converts a hypothetical route into a numeric BQ
  penalty

### Phase 2: Integrate into main candidate scoring

Files:

- `libs/s25main/ai/aijh/planning/AIConstruction.cpp`

Actions:

- call the helper for every valid `tmpRoute`,
- add the returned penalty into the main candidate score,
- keep the rest of the selection logic unchanged.

Output:

- the AI still favors short useful connections, but now slightly prefers routes
  that preserve better building space

### Phase 3: Integrate into alternative-road scoring

Files:

- `libs/s25main/ai/aijh/planning/AIConstruction.cpp`

Actions:

- compute BQ penalty for candidate shortcut roads,
- fold the penalty into the "worth it?" comparison,
- keep the existing threshold-based design.

Output:

- alternative roads stop cutting through valuable plots just to save a modest
  distance

### Phase 4: Verify and tune

Files:

- targeted tests under `tests/s25Main/integration/` or `tests/s25Main/simple/`
- optional docs update under `docs/ai/`

Actions:

- add focused tests for route ranking when one candidate damages more BQ than
  another,
- validate no behavior regressions in obvious "only one legal route" cases,
- tune `BQ_PENALTY_WEIGHT` and the BQ value table based on test and replay
  results.

Output:

- stable, explainable penalty behavior

## Testing Plan

### Unit/integration cases to add

1. Two legal candidate flags, same rough length:
   - one route downgrades a `Castle` plot to `Flag`,
   - the other only touches `Flag`-grade space,
   - expect the less damaging route to win.

2. Slightly shorter but damaging route vs slightly longer but clean route:
   - confirm the new weight is light, not dominant,
   - choose route lengths so the winner validates intended tuning.

3. Only one legal route:
   - BQ penalty must not cause failure when no alternative exists.

4. Alternative road shortcut:
   - ensure a damaging shortcut is rejected when its distance gain is too small.

5. Overlapping road footprint:
   - confirm affected plots are deduplicated and not charged multiple times.

### Replay/manual checks

- Observe AI road choices in dense settlement areas with many nearby flags.
- Compare before/after behavior near future large-building spots.
- Confirm the AI still connects new buildings reliably and does not become too
  hesitant.

## Risks and Open Questions

### 1. Performance

This runs inside candidate scoring, so it may execute several times per
connection attempt.

Mitigation:

- keep the affected-plot footprint small,
- deduplicate points,
- avoid heap-heavy structures where possible,
- add the feature first in the main path only if profiling shows the
  alternative-road path is too expensive.

### 2. Enum semantics of `Mine`

`Mine` does not fit a simple "bigger is better" ladder.

Mitigation:

- use an explicit penalty table,
- tune `Mine` separately if tests show the initial value is wrong.

### 3. Alignment with actual world updates

The recommended footprint is based on `RecalcBQForRoad()`. If there are
second-order BQ effects beyond that set, the heuristic may miss them.

Mitigation:

- start with the exact world update footprint,
- expand only if verified by tests or debug output.

### 4. Tuning may be subjective

"Light penalty" can mean different things depending on map density.

Mitigation:

- start with low weight,
- keep the value table explicit and easy to tune,
- prefer small, explainable constants over opaque formulas.

## Recommended First Cut

For the first implementation, keep the scope narrow:

- add one read-only route-BQ-penalty helper,
- use the `RecalcBQForRoad()` footprint exactly,
- use the explicit BQ value table above,
- integrate it into both main-road and alternative-road scoring,
- start with `BQ_PENALTY_WEIGHT = 1`.

That is small enough to implement safely, grounded in the current world/BQ
rules, and likely to improve the "far flag chosen over obviously better nearby
flag" cases without overhauling the planner.
