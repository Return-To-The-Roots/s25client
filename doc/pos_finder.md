# PositionFinder Overview

`AIJH::PositionFinder` is the micro-service the JH AI uses to scout fresh
construction sites. It sits next to `AIConstruction` and `BuildingPlanner`
and evaluates terrain, proximity constraints, and resource overlays before a
build job settles on a final `MapPoint`.

## Responsibilities
- Gather anchor positions (current military buildings plus the HQ) and
  search rings around them for promising sites.
- Enforce proximity rules derived from `AIConfig::locationParams`, avoiding
  duplicate infrastructure or overcrowded storehouses.
- Query resource maps (wood, plant space, fish, stones) and rate candidates
  with heuristics tuned per building type.
- Validate that resources are actually harvestable by checking fishable
  coast tiles and granite nodes reachable via human paths.

## Main Entry Points
- `FindBestPosition(BuildingType)`: iterates over anchor positions and keeps
  the highest-rated candidate returned by `FindPositionAround`.
- `FindPositionAround(type, around, radius)`: applies building-specific
  heuristics after a general proximity check. Special cases:
  - **Woodcutter** – scores wood density and boosts locations near foresters.
  - **Forester** – prefers plant space with nearby woodcutters.
  - **Farm** – targets rich plant space away from foresters.
  - **Quarry** – requires confirmable granite nodes via `ValidStoneInRange`.
  - **Fishery** – searches coast-adjacent water with reachable fish spots.
  - **Default** – falls back to `AIPlayerJH::SimpleFindPosition` when no
    dedicated heuristic exists.
- `CheckProximity(type, pt)`: enforces per-building minimal radii around
  other structures, short-circuiting early for overlapping storehouses.
- `FindBestPositions(...)`: wraps `AIResourceMap::findBestPositions` after
  refreshing the relevant resource mask around the reference node.
- `ValidFishInRange` / `ValidStoneInRange`: verify that the chosen tile has
  accessible resources (fish or granite) within a bounded search radius and
  that a worker path exists.

## Usage Notes
- `PositionFinder` assumes `AIConstruction` tracks “usual” buildings and
  exposes helper queries such as `OtherUsualBuildingInRadius` and
  `OtherStoreInRadius`.
- Ratings returned by `FindPositionAround` combine raw resource scores with
  bonuses (foresters near woodcutters, woodcutters near foresters, etc.), so
  callers should treat only the final `MapPoint` as significant and ignore
  the absolute rating value.
- When no valid site is found the class returns `MapPoint::Invalid`, letting
  higher-level build jobs reschedule or downgrade/upgrade their plans.

## Configuration
- Woodcutter/forester synergies use the `posFinder.<Building>.rating` block
  from the AI weights YAML. Each entry accepts `radius` (tiles to check) and
  `multiplier` (bonus per neighbor). Example:

```
posFinder:
  Woodcutter:
    rating:
      Forester:
        radius: 7        # default search radius
        multiplier: 300  # default bonus per forester
  Forester:
    rating:
      Woodcutter:
        radius: 6        # default search radius
        multiplier: 50   # default bonus per woodcutter
```

- Omitting the block keeps the compiled defaults (7/300 and 6/50), so only
  add entries when tuning the AI’s building placement heuristics.
