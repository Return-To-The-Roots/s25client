# AI Attack Target Selection

Most of the orchestration still lives in `AIPlayerJH.cpp`, but the target
selection logic has been extracted into
`ai/aijh/combat/TargetSelector.cpp` so it can evolve independently of the
combat-resolution code.

## Trigger Flow

- Every few hundred gameframes the AI runs `UpdateCombatMode()` (lines
  1784-1834) to keep an internal `attackMode` between Defense and Attack
  based on the global combat fulfillment level configured in
  `AIConfig`. When `(gf + playerId * 17) % attack_interval == 0`,
  `TryToAttack()` executes (lines 1884-1972).

## Discovering Candidate Targets

1. `AIPlayerJH::SelectAttackTarget(TargetSelectionMode::Random)` (in
   `TargetSelector.cpp`) iterates the AI’s military buildings while applying
   the same heuristics as before: skip inland posts, cap the scan to roughly
   40 per tick, and look for nearby enemy buildings via
   `gwb.LookForMilitaryBuildings(src, 2)`.
2. Candidates remain subject to the visibility, attackable-enemy, distance
   (`BASE_ATTACKING_DISTANCE`), and “not newly built” checks.
3. HQs or harbors without soldiers are still inserted at the front, while
   every other target lands at the back.
4. Only the non-prioritized suffix is shuffled, so opportunistic steals keep
   their precedence even though the rest of the list is randomized.
5. `SelectAttackTargetRandom()` evaluates the ordered candidates immediately
   and returns the first viable target.
6. `SelectAttackTargetPrudent()` uses the same discovery helper but applies
   additional heuristics (see below) before settling on a candidate.
7. The active selection mode is configured via `combat.targetSelection`
   in the AI weights config (defaults to `Random`).
   A sample `combat` block:

   ```yaml
   combat:
     fulfillment:
       low: 4.0
       medium: 8.0
       high: 12.0
     attackIntervals:
       easy: 2500
       medium: 750
       hard: 100
    targetSelection: Prudent   # Random | Prudent | Biting | Attrition
   ```
8. `TryToAttack()` now simply asks for a target via the configured mode and
   issues `aii.Attack`/`TrackCombatStart` when a plan is available.

## Evaluating Candidates

While evaluating potential targets `SelectAttackTargetRandom()` performs:

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
5. If all checks pass the method returns the target to `TryToAttack()`,
   which then re-counts nearby attackers before calling
   `aii.Attack(dest, attackersCount, true)` and `TrackCombatStart`.
6. The helper stops after finding the first valid attack; any remaining
   potential targets are reconsidered during the next interval.

When running with `TargetSelectionMode::Prudent` the selector:

1. Starts from the same candidate list but skips buildings already under
   attack.
2. Requires the AI to outnumber the defenders by at least two soldiers before
   keeping a target.
3. Keep only the entries with the lowest defender count.
4. If multiple choices remain, estimate each target’s counterattack risk by
   summing the enemy soldiers (beyond the single mandatory defender) located
   within `BASE_ATTACKING_DISTANCE` and keep the lowest totals.
5. If ties still remain, shuffle the survivors and pick one at random.

When running with `TargetSelectionMode::Biting` the selector:

1. Uses the same candidate discovery pass, ensuring each target still has at
   least one available attacker and, on Hard difficulty, enough aggregate
   strength to overpower enemy garrisons.
2. Keeps the Random-mode defense-mode restriction, so retaking lost or
   isolated forts still applies before evaluating priorities.
3. Immediately returns any valid Headquarters target, treating it as maximum
   priority regardless of collateral score.
4. Otherwise queries a weighted capture-loss score and chooses the building
   whose capture would destroy the highest-scoring set of dependent enemy
   structures. Each destroyed building contributes
   `combat.buildingScores[BuildingType]` points, defaulting to `1` when the
   type is not overridden in YAML. Ties fall back to the first candidate that
   satisfied the constraints, so a target is always returned when at least one
   viable option exists.

When running with `TargetSelectionMode::Attrition` the selector:

1. Pulls the latest `StatisticType::Military` values for the AI and every
   attackable opponent and refreshes `MilitaryStatsHolder` densities for the
   AI player. It defers to the Biting heuristic only if both checks pass:
   enough global force advantage (`combat.forceAdvantageRatio`) and sufficient
   near-frontier troop density
   (`combat.minNearTroopsDensity` against `densityNear`).
2. Otherwise it enumerates the same candidate list but first filters for
   `nobMilitary` buildings that were originally owned by the AI
   (`GetOriginOwner`). The AI prefers retaking these forts.
3. Within each pool it prioritizes targets captured within the last
   2000 gameframes (`GetCapturedGF`) when reclaiming former structures, then
   minimizes the number of defenders.
4. If there are no recapture targets it repeats the process across the full
   candidate list but uses a tighter 1000-gameframe window before again
   selecting the option with the smallest defender count.

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
