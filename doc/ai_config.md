# AIConfig Summary

`AIConfig` describes the configurable behaviour for AI-controlled players. It
collects per-building weight parameters, location heuristics, combat pacing,
tool priorities, and explicit building disables. The structure is defined in
`libs/s25main/ai/aijh/AIConfig.h` and populated through YAML in
`libs/s25main/ai/aijh/AIConfig.cpp`.

## Structure

- `wantedParams[BuildingType]` – Desired counts and production weights that the
  planner uses to decide whether another building of a given type is needed.
- `locationParams[BuildingType]` – Position-finding weights that rate how
  attractive a potential site is (terrain access, distance modifiers, etc.).
  Includes `resources` thresholds for minimal required AI resource values per
  building placement heuristic.
- `combat` – A `CombatConfig` block holding:
  - `fulfillmentLow/Medium/High` thresholds that describe how many soldiers the
    AI wants on hand before escalating attack plans.
  - `forceAdvantageRatio` – Attrition-mode multiplier against the strongest
    attackable enemy's military stat before switching to biting target
    selection.
  - `minNearTroopsDensity` – Minimum
    `MilitaryStatsHolder::densityNear` value required before attrition-mode AI
    is allowed to switch to biting target selection.
  - `attackIntervals[AI::Level]` – Minimum frames between attack attempts per
    AI difficulty (easy/medium/hard).
  - `targetSelection` – Algorithm used to select objectives. Options are
    `Random`, `Prudent`, `Biting`, or `Attrition`.
- `disableBuilding` – Vector of `BuildingType` values that should start in the
  “disabled” state for the owning player. `GamePlayer` checks this list when it
  initializes (and when loading saves) and flips the corresponding entries in
  `building_enabled` to `false` so those blueprints cannot be queued.
- `toolPriority` – Per-tool priority values used by `AIPlayerJH::AdjustSettings`
  when managing tool production. Defaults to the hardcoded `TOOL_PRIORITY`
  table unless overridden.
- `distributionParams[GoodType][BuildingType]` – Distribution adjuster tuning
  per distribution edge (`distributed good -> target building`). Each
  `DistributionParams` entry exposes `overstockingPenalty[GoodType]` as
  `BuildParams`, so penalties can use `constant`, `linear`, `exponential`,
  `logTwo`, `min`, and `max`.

## YAML Configuration

`applyWeightsCfg()` loads AI configuration files via YAML and applies the
following top-level sections if present:

| Key             | Description                                                                           |
|-----------------|---------------------------------------------------------------------------------------|
| `posFinder`     | Building-name map whose values feed `Weights::parseLocationParams` per entry (including `resources`, `proximity`, `rating`, `buildOnBorder`). |
| `buildPlanner`  | Building-name map parsed through `Weights::parseWantedParams` to update `wantedParams`.|
| `combat`        | Optional object containing `fulfillment`, `forceAdvantageRatio`, `minNearTroopsDensity`, `attackIntervals`, and `targetSelection`. |
| `disableBuilding` | Sequence of building names (matching `BUILDING_NAME_MAP` keys) to disable entirely. |
| `toolPriority`  | Map of tool names to signed priority values (e.g. `Tongs: 2`). Missing entries keep defaults. |
| `distributionAdjuster` | Map of distributed goods to target buildings. Each `distributedGood -> building` node may define `overstockingPenalty` as a map of stock goods to `BuildParams`. |

Invalid entries log warnings but leave defaults untouched. Player-specific
overrides can be loaded with `ApplyPlayerWeightsCfg`, which stores a dedicated
`AIConfig` instance per player ID.

Example:

```yaml
distributionAdjuster:
  Fish:
    CoalMine:
      overstockingPenalty:
        Coal:
          min: 50
          linear: -0.02
    GoldMine:
      overstockingPenalty:
        Coal:
          min: 35
          linear: -0.04
```

With these settings:
- `Fish -> CoalMine` drops by 1 for each 50 coal over 50 stock.
- `Fish -> GoldMine` drops by 1 for each 25 coal over 35 stock.

## Usage Notes

- `GetAIConfigForPlayer()` returns either the per-player override or the global
  `AI_CONFIG` singleton. Both AI planners (`AIPlayerJH`) and `GamePlayer` query
  this data.
- The combat block defaults to attack intervals of 2500/750/100 frames
  (easy/medium/hard), `minNearTroopsDensity = 1.0`, and uses random target
  selection until overridden.
- `disableBuilding` defaults to an empty list; omitting it in YAML preserves the
  original behaviour where all buildings start enabled.
