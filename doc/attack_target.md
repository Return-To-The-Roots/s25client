# AI Attack Target Selection

Most of the orchestration still lives in `AIPlayerJH.cpp`, but the target
selection logic has been extracted into `ai/aijh/TargetSelector.cpp` so it
can evolve independently of the combat-resolution code.

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
     targetSelection: Prudent   # or "Random" (default)
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
