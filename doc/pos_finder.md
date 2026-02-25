# Position Finding Overview

The current AI position search is implemented by
`AIJH::GlobalPositionFinder` (`libs/s25main/ai/aijh/GlobalPositionFinder.cpp`).
It does a full-map scan and returns one best point for a requested
`BuildingType`.

## Entry Point

- `GlobalPositionFinder::FindBestPosition(BuildingType bt)`

Called by `AIPlayerJH::FindBestPosition`, and used by global build jobs.

## Global Scan Pipeline

For each map point, the finder applies filters in this order:

1. AI node ownership/reachability/farm state:
   - reject if `!node.reachable || !node.owned || node.farmed`
2. Building size fit:
   - reject if `!canUseBq(node.bq, BUILDING_SIZE[bt])`
3. Harbor proximity:
   - reject if `isHarborPosClose(pt, 2, true)` for non-harbor buildings
4. Border rule:
   - `locationParams[bt].buildOnBorder == false` rejects points with
     `Borderland > 1`
5. Minimal resource requirements:
   - checked before rating via `locationParams[bt].minResources[...]`
6. Point rating and proximity:
   - `GetPointRating(bt, pt)` may reject the point (`std::nullopt`)

The highest positive rating wins (`bestValue` starts at `0`), otherwise
`MapPoint::Invalid()` is returned.

## Resource Requirement Check

`MeetsPointResourceRequirements(...)` is data-driven:

- It iterates all `AIResource` values.
- For each resource with `locationParams[building].minResources[resource] > 0`,
  it requires `CalcResourceValue(point, resource) >= minResources[resource]`.
- If all configured minimums pass, the point is accepted.

## Rating Logic

`GetPointRating(...)` now has three stages:

1. `CheckProximity(...)` (may reject point)
2. Building-specific hard constraints (switch-case rejections only)
3. Generic rating from config:
   - base rating from `locationParams[type].resourceRating`
   - sum of configured neighborhood bonuses from `locationParams[type].rating[...]`
   - minus `resourcePenalty[...]` values

Resource base rating and bonus defaults are not hardcoded in the switch anymore.
They are read from `LocationParams`.

The switch currently only applies special constraints:

- `Woodcutter`: reject if another woodcutter in radius `3`
- `Farm`: reject if forester in radius `8`
- `Quarry`: requires `ValidStoneinRange(...)`
- `Fishery`: reject if fishery nearby in radius `5` and requires `ValidFishInRange(...)`

## Proximity Logic (Current Behavior)

`CheckProximity(...)` reads `locationParams[type].proximity[otherType]`.
For an enabled rule it computes:

- `minRadius = CALC::calcCount(numBuildingsOfType, proximity.minimal)`

Then it checks either:

- `OtherStoreInRadius` for `otherType == Storehouse`
- `OtherUsualBuildingInRadius` otherwise

Important current detail: the implementation returns on the first enabled
proximity rule it encounters in enum iteration, so it does not combine all
enabled proximity rules in one pass.

## Resource Availability Helpers

- `ValidFishInRange(pt)`
  - scans radius `5` for fish resource nodes and requires a short human path
    (`FindHumanPath(pt, nb, 10)`) to a neighboring water node.
- `ValidStoneinRange(pt)`
  - scans rings up to radius `8` for granite nodal objects and requires human
    path reachability (`FindHumanPath(pt, granitePt, 20)`).

## Config Knobs

Position behavior is configured through `posFinder` / `locationParams`:

- `buildOnBorder` (`bool`)
- `resources` (`minResources` per `AIResource`)
- `resourceRating`:
  - `resource` (`AIResource`) for base `CalcResourceValue(...)`
  - `defaultRadius` fallback for enabled `rating` entries without explicit radius
  - `defaultMultiplier` fallback for enabled `rating` entries without explicit multiplier
- `resourcePenalty` (per-resource `BuildParams`, applied for all building types)
- `proximity` (dynamic radius via `BuildParams`)
- `rating` (per-neighbor type `radius` + `multiplier`)

Example (woodcutter/forester rating override):

```yaml
posFinder:
  Woodcutter:
    resourceRating:
      resource: Wood
      defaultRadius: 7
      defaultMultiplier: 300
    rating:
      Forester:
        # radius/multiplier omitted -> uses resourceRating defaults
  Forester:
    resourceRating:
      resource: Plantspace
      defaultRadius: 6
      defaultMultiplier: 50
    rating:
      Woodcutter:
        # radius/multiplier omitted -> uses resourceRating defaults
  Well:
    resourcePenalty:
      Stones:
        linear: 0.025
```
