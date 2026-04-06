# Enemy Building Reachability Calculation

This document explains how enemy military building "reachability" is calculated in
`nobMilitary::LookForEnemyBuildings` (`libs/s25main/buildings/nobMilitary.cpp:343`).

## Scope and Inputs

The function runs for one military building (`this`) and can optionally ignore one
building (`exception`) to avoid feedback loops during destruction/recalculation.

Main inputs:

- current building position: `pos`
- candidate set from military grid: `world->LookForMilitaryBuildings(pos, 3)`
- geometric distance: `world->CalcDistance(pos, other->GetPos())`
- add-on toggle:
  `AddonId::FRONTIER_DISTANCE_REACHABLE`

Related constants:

- `BASE_ATTACKING_DISTANCE = 17`
- `EXTENDED_ATTACKING_DISTANCE = 1`
- `MAX_ATTACKING_RUN_DISTANCE = 40`

Defined in `libs/s25main/gameData/MilitaryConsts.h`.

## Step 1: Candidate Collection

Candidates are obtained from:

- `world->LookForMilitaryBuildings(pos, 3)`

This is a military-square prefilter (`MilitarySquares`), not a strict circle by
exact map distance. Exact distance is checked afterward per candidate.

## Step 2: Enemy/Attackable Filtering

Each candidate is considered only if all conditions hold:

- not equal to `exception`
- belongs to a different player
- owning player is attackable:
  `world->GetPlayer(candidate->GetPlayer()).IsAttackable(player)`

## Step 3: Initial Frontier Class by Geometry

For each valid candidate, the code computes `newFrontierDistance`:

1. `Near` if military radii overlap:
   - `distance <= myMilitaryRadius + enemyMilitaryRadius`
2. else `Mid` if candidate is in theoretical attack range from this building:
   - `distance < BASE_ATTACKING_DISTANCE + (GetMaxTroopsCt() - 1) * EXTENDED_ATTACKING_DISTANCE`
3. else, for normal military buildings (`GO_Type::NobMilitary`), also try enemy-side
   troop range:
   - `distance < BASE_ATTACKING_DISTANCE + (enemy.GetMaxTroopsCt() - 1) * EXTENDED_ATTACKING_DISTANCE`
4. otherwise `Far`.

Why two `Mid` checks:

- A building may be out of this building's theoretical attack range but still be in
  the enemy building's range, so both sides are considered.

## Step 4: Reachability Validation (Optional Add-on)

If add-on `FRONTIER_DISTANCE_REACHABLE` is enabled, and the candidate is at least
`Mid`, the code performs a path check:

- `DoesReachablePathExist(*world, candidatePos, pos, MAX_ATTACKING_RUN_DISTANCE)`

Implementation:

- `libs/s25main/pathfinding/FindPathReachable.cpp`
- uses `PathConditionReachable`
  (`libs/s25main/pathfinding/PathConditionReachable.h`)

Important behavior:

- This check validates terrain-reachable walk paths.
- It rejects permanently blocked terrain transitions (e.g. sea/lava/unreachable).
- It does not apply diplomatic/ownership combat policy logic; it is a terrain
  reachability test.

If no reachable path exists, the candidate is downgraded to `Far`.

## Step 5: Apply Result

For each checked candidate:

- own `frontier_distance` keeps the strongest (closest) classification:
  - `if(newFrontierDistance > frontier_distance) frontier_distance = newFrontierDistance;`
- if candidate is a military building type, notify it:
  - `static_cast<nobMilitary*>(candidate)->NewEnemyMilitaryBuilding(newFrontierDistance);`

This propagates updated frontier pressure to nearby military buildings.

## Step 6: Harbor Override

After candidate loop:

- if current result is `Far` or `Mid`
- and `SEA_ATTACK` add-on is enabled
- and harbor is nearby (`CalcDistanceToNearestHarbor(pos) < SEAATTACK_DISTANCE + 2`)

then frontier is set to `Harbor`.

## Step 7: Troop Regulation

At the end, `RegulateTroops()` is called, so the resulting frontier classification
immediately affects garrison requirements and soldier orders.

## Summary Formula

For each enemy candidate:

1. classify by geometric relation (`Near`/`Mid`/`Far`)
2. optionally validate with terrain pathfinder (addon)
3. degrade to `Far` if unreachable
4. keep strongest distance for this building
5. propagate to other military buildings
6. apply harbor override
7. regulate troops

