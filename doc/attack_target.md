# AI Attack Target Selection

Reference: `libs/s25main/ai/aijh/AIPlayerJH.cpp`.

## Trigger Flow

- Every few hundred gameframes the AI runs `UpdateCombatMode()` (lines
  1784-1834) to keep an internal `attackMode` between Defense and Attack
  based on the global combat fulfillment level configured in
  `AIConfig`. When `(gf + playerId * 17) % attack_interval == 0`,
  `TryToAttack()` executes (lines 1884-1972).

## Discovering Candidate Targets

1. Iterate over the AI’s own military buildings via
   `aii.GetMilitaryBuildings()`.
   - Buildings flagged as `FrontierDistance::Far` (pure inland posts) are
     skipped.
   - When the AI owns more than 40 buildings it randomly skips entries so
     that roughly `limit=40` candidates are inspected per pass.
2. For each usable building, `gwb.LookForMilitaryBuildings(src, 2)` scans
   enemy posts within the base attacking radius.
3. A target is considered only if all of these are true:
   - It is visible to the AI player (`aii.IsVisible(dest)`).
   - It belongs to an attackable enemy (`aii.IsPlayerAttackable`).
   - Distance from the source building is strictly less than
     `BASE_ATTACKING_DISTANCE` (21 nodes; see
     `libs/s25main/gameData/MilitaryConsts.h`).
   - For enemy `nobMilitary` buildings, `IsNewBuilt()` must be false so
     freshly placed huts get a short grace period.
4. Headquarters or harbor buildings **without defenders** are inserted at
   the front of the `potentialTargets` list so the AI tries to steal them
   first; all other candidates are pushed to the back.
5. After scanning all sources, the non-prioritized portion of
   `potentialTargets` is shuffled with a PRNG so that no single enemy
   building is always preferred when several options exist.

## Evaluating Candidates

For each target in the ordered list:

1. Gather possible attackers using `gwb.LookForMilitaryBuildings(dest, 2)`.
   - Only the AI’s own `nobMilitary` buildings contribute.
   - Buildings currently under attack are ignored.
   - `GetSoldiersStrengthForAttack(dest, newAttackers)` reports how many
     soldiers this building can send and their combined strength, which is
     accumulated across all contributors.
2. Skip targets if `attackersCount == 0`.
3. On **Hard** difficulty the AI requires
   `attackersStrength > enemyStrength + 2` for enemy military posts and
   refuses to attack empty garrisons (it expects defenders).
4. When currently in Defense mode the AI calls
   `CanAttackInDefenseMode()`:
   - Only proceed if it either recently lost the exact building
     (`IsRecentlyLostMilitaryBuilding`) **or** the target is a “lonely”
     stronghold (no second enemy military building within radius 12, see
     `IsLonelyEnemyStronghold`).
   - The attacking force must outnumber the defenders (or, for warehouses
     and HQs, only attack if they have zero defenders).
5. If all checks pass the AI issues `aii.Attack(dest, attackersCount, true)`
   to launch the mission and records the combat start via
   `TrackCombatStart` for later bookkeeping.
6. The loop stops after ordering the first valid attack; any remaining
   potential targets are reconsidered during the next interval.

## Key Takeaways

- Only frontline military buildings scout for targets, keeping inland
  garrisons focused on defense.
- Targets must be visible, within the base 21-node range, and not freshly
  constructed.
- HQs or harbors without soldiers are opportunistically prioritized.
- Attacks require enough nearby contributors; Hard AI compares aggregate
  strength before committing.
- Defense mode further restricts attacks to retaking lost posts or
  punishing isolated enemy fortifications.
