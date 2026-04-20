# AI Attack Target Selection

Most of the orchestration still lives in `AIPlayerJH.cpp`, but the target
selection logic has been extracted into
`ai/aijh/combat/TargetSelector.cpp` so it can evolve independently of the
combat-resolution code.

## Trigger Flow

- Every attack interval the AI calls `TryToAttack()`. The controller uses the
  configured target-selection algorithm directly; there is no separate combat
  posture state anymore.

## Discovering Candidate Targets

1. `AIPlayerJH::SelectAttackTarget(...)` (in `TargetSelector.cpp`) iterates
   the AI’s military buildings while applying
   the same heuristics as before: skip inland posts, cap the scan to roughly
   40 per tick, and look for nearby enemy buildings via
   `gwb.LookForMilitaryBuildings(src, 2)`.
2. Candidates remain subject to the visibility, attackable-enemy, distance
   (`BASE_ATTACKING_DISTANCE`), and “not newly built” checks.
3. HQs or harbors without soldiers are still inserted at the front, while
   every other target lands at the back.
4. `SelectAttackTargetPrudent()` uses the same discovery helper but applies
   additional heuristics (see below) before settling on a candidate.
5. The active selection mode is configured via `combat.targetSelection`
   in the AI weights config (defaults to `Prudent`).
   A sample `combat` block:

   ```yaml
   combat:
     attackIntervals:
       easy: 2500
       medium: 750
       hard: 100
    targetSelection: Prudent   # Prudent | Biting | Attrition
   ```
6. `TryToAttack()` now simply asks for a target via the configured mode and
   issues `aii.Attack`/`TrackCombatStart` when a plan is available.

## Evaluating Candidates

While evaluating potential targets the controller:

1. Gather possible attackers using `gwb.LookForMilitaryBuildings(dest, 2)`.
   - Only the AI’s own `nobMilitary` buildings contribute.
   - Buildings currently under attack are ignored.
   - `GetSoldiersStrengthForAttack(dest, newAttackers)` reports how many
     soldiers this building can send and their combined strength, which is
     accumulated across all contributors.
2. Skip targets if `attackersCount == 0`.
3. On **Hard** difficulty the AI requires
   `attackersStrength > enemyStrength + 2` for defended enemy military posts.
   Empty military garrisons are exempt from that strength gate and can be
   attacked as soon as at least one attacker is available.
4. If all checks pass the method returns the target to `TryToAttack()`,
   which then re-counts nearby attackers before calling
   `aii.Attack(dest, attackersCount, true)` and `TrackCombatStart`.
5. The helper stops after finding the first valid attack; any remaining
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
   strength to overpower enemy garrisons that still have defenders.
2. Immediately returns any valid Headquarters target, treating it as maximum
   priority regardless of collateral score.
3. Otherwise, any enemy military building with zero stationed defenders is
   promoted ahead of the collateral-score heuristic, so the AI snaps it up as
   soon as any attacker can be sent.
4. Otherwise queries a weighted capture-loss score and chooses the building
   whose capture would destroy the highest-scoring set of dependent enemy
   structures. Each destroyed building contributes
   `combat.buildingScores[BuildingType]` score, defaulting to `1.0` when the
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
- Empty enemy military posts are snapped up immediately once the AI can send
  any attacker.
- Other attacks still require enough nearby contributors; Hard AI compares
  aggregate strength before committing against defended garrisons.
