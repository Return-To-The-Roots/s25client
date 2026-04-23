# AI Troops Limiting

The JH AI does not move soldiers directly. It influences garrisoning through
two control layers in `libs/s25main`:

- global military settings (`ChangeMilitary`), which determine the desired
  garrison size from the building's frontier classification, and
- per-building total troop caps (`SetTotalTroopLimit`), which clamp that
  desired size before `nobMilitary::RegulateTroops()` orders or ejects
  soldiers.

The runtime logger in `TroopsLimitEventLogger.cpp` only records the second
layer: changes to `nobMilitary::total_troop_limit`.

See also:

- `docs/development/event-loggers.md`
- `docs/gameplay/military/deployment.md`
- [configuration.md](configuration.md) — `troopsDistribution.strategy`,
  `troopsDistribution.frontierMultipliers`, and `combat.buildingScores`
  feeding the distribution rule below.
- [attack-target-selection.md](attack-target-selection.md) —
  `combat.buildingScores` is shared with the Biting target selector.
- [gold-distribution.md](gold-distribution.md) — adjacent military
  control surface (coin enable/disable per building).
- [performance-profiling.md](performance-profiling.md) —
  `UpdateTroopsLimit*` profiler sections cover this code path.

## Engine Contract

For every military building, `nobMilitary::CalcRequiredNumTroops()` computes:

```text
required = min(frontier-based target, total_troop_limit)
```

The frontier-based target comes from one of the player's military sliders:

- setting `4`: `Far`
- setting `5`: `Mid`
- setting `6`: `Harbor`
- setting `7`: `Near`

The formula always keeps at least one soldier:

```text
1 + (maxTroops - 1) * settingValue / scale
```

`RegulateTroops()` then compares the building's actual soldiers against that
required count. If the building has too many troops it sends some home; if it
has too few and road access exists it orders replacements from warehouses.

This means the AI's troop-limiting logic is declarative: it sets targets and
caps, and the regular military-building regulation loop enforces them.

## Active AI Path

The current AI loop in `AIPlayerJH::RunGF()` updates troop-related policy in
two periodic passes:

- every `150` gameframes: `AdjustSettings()`
- every `1000` gameframes, staggered by `playerId`:
  `AIMilitaryLogistics::UpdateTroopsLimit()`

Those two passes are the active mechanism used by the shipped AI.

## Global Military Settings

`AIEconomyController::AdjustSettings()` rebuilds parts of the player's
`MilitarySettings` array and sends `ChangeMilitary(...)` when relevant values
changed.

The current policy is:

- setting `1` (strength preference): `5` if the AI has at least one non-`Far`
  military building, otherwise `0`
- setting `4` (`Far` garrisons): `8` only if an "upgrade building" exists and
  the economy can feed coin upgrades, otherwise `0`
- setting `5` (`Mid` garrisons): chosen dynamically by `CalcMilSettings()`
- setting `6` (`Harbor` garrisons): `8` when the `SEA_ATTACK` add-on is
  enabled, otherwise `0`
- setting `7` (`Near` garrisons): always `8`

In practice this makes the AI keep border buildings full, keep harbor
buildings full only in sea-attack games, and usually reduce interior/far
pressure when soldiers are scarce.

### Mid-frontier setting selection

`CalcMilSettings()` estimates how many soldiers would be tied up by different
`Mid` slider values:

- `Near` and `Harbor` buildings are treated as fixed high-priority consumers.
- `Far` buildings count either as a single-soldier placeholder or as
  high-priority consumers in a few legacy upgrade-related cases.
- `Mid` buildings are evaluated for slider values `4` through `8`.

The function then picks the highest `Mid` setting that still leaves a reserve.
The main threshold is roughly "do not commit more than 10/11 of all soldiers".
If the current slider is already high, the function also allows holding that
value when the available soldier count can still cover the resulting demand.

## Per-Building Total Cap Redistribution

`AIMilitaryLogistics::UpdateTroopsLimit()` is the part that directly feeds
`TroopsLimitEventLogger`.

The algorithm is:

1. Collect all owned military buildings that are not `Far`.
2. Count available soldiers in warehouses.
3. Add the soldiers already stationed in those eligible buildings.
4. Compute the combined capacity of the eligible buildings.
5. Assign each eligible building a valuation score according to
   `AIConfig::troopsDistribution.strategy`.
6. Distribute the available soldiers as per-building `total_troop_limit`
   values, while keeping a baseline minimum of one soldier per eligible
   building.

The built-in strategies are:

- `Fair`: every eligible building gets score `1`
- `ProtectedBuildingValue`: score equals the summed
  `combat.buildingScores` of the buildings that would be lost if that military
  building were captured, multiplied by the configured
  `troopsDistribution.frontierMultipliers[FrontierDistance]`

The distribution rule is intentionally simple:

- every eligible building starts at limit `1`
- the remaining distributable soldiers are assigned one by one
- each extra soldier goes to the building with the lowest currently assigned
  extras-to-score ratio
- ties are broken by a rotating scan start
  `(currentGF / 1000 + playerId) % numEligibleBuildings`

Effects of this policy:

- the AI never intentionally drives an eligible military building below one
  soldier
- under `Fair`, non-`Far` garrisons are spread evenly instead of being filled
  greedily
- under `ProtectedBuildingValue`, buildings that protect more valuable nearby
  infrastructure receive a larger share of the surplus, with optional
  frontier-based weighting from config
- the rotating start index prevents the same building from always winning
  equal-score ties

When the computed cap differs from the current one, the AI sends
`SetTotalTroopLimit(...)`. `nobMilitary::SetTotalTroopLimit()` clamps the value
to `[1, GetMaxTroopsCt()]`, logs the change, and immediately reruns
`RegulateTroops()`.

## Border Events And Legacy Per-Rank Logic

There is older AI code that manipulates per-rank `troop_limits[rank]` via
`SetTroopLimit(...)`, but that is not the main active path anymore.

Relevant pieces:

- `AIEventHandler::HandleBorderChanged()` restores every per-rank limit to the
  building maximum when a military building becomes non-`Far`, and also
  re-enables coin delivery there.
- `AIMilitaryLogistics::MilUpgradeOptim()` contains older logic for temporarily
  flushing or reshaping garrisons with per-rank commands.

Two details make that legacy path effectively dormant today:

- the call to `MilUpgradeOptim()` in `AIPlayerJH::RunGF()` is commented out
- `AIEconomyController::UpdateUpgradeBuilding()` currently returns `-1`, so the
  upgrade-special-case path is not selected

So if you are analyzing current AI matches, the important troop-limiting
behaviour is the combination of `ChangeMilitary(...)` plus
`UpdateTroopsLimit()`, not the older per-rank reshuffling code.

## What `troops_limit_log.csv` Actually Shows

`TroopsLimitEventLogger` writes:

```text
gameframe,playerId,newLimit,buildingType,buildingId
```

For AI-controlled players, rows in this file correspond to changes in
`total_troop_limit`, usually coming from `AIMilitaryLogistics::UpdateTroopsLimit()`.

Important limitation:

- `ChangeMilitary(...)` does not produce entries in `troops_limit_log.csv`
- `SetTroopLimit(...)` does not produce entries either

So the logger shows only one part of the AI's troop-allocation policy: the
per-building total-cap redistribution. To explain why a building wanted more or
fewer soldiers than its logged cap, you must also consider the global military
settings in effect for its `FrontierDistance`.
