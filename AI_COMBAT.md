# AI Combat Logic in `AIPlayerJH`

## Tuning Sources
- Combat thresholds and attack cadence now live in `AI_CONFIG.combat` (see `libs/s25main/ai/aijh/AIConfig.cpp:15`). Defaults:
  - Fulfillment levels: low `4.0`, medium `8.0`, high `12.0`.
  - Minimum near-frontier troops density for attrition-to-biting switch: `1.0`.
  - Attack intervals (frames): Easy `2500`, Medium `750`, Hard `100`.
- Optional YAML overrides can be supplied under a `combat` section with `fulfillment.low|medium|high`,
  `forceAdvantageRatio`, `minNearTroopsDensity`, and `attackIntervals.easy|medium|hard`.

## Update Cadence
- The AI runs its main loop in `AIPlayerJH::RunGF`. Every 500 game frames (offset per player) it refreshes the combat stance via `UpdateCombatMode`, using the configured fulfillment levels.
- Attack attempts are issued when `(gf + playerId * 17) % attack_interval == 0`; `attack_interval` is read from `AI_CONFIG.combat` per difficulty level.
- Sea assaults are tried with the same cadence but offset by an additional 41 frames, and only if the `SEA_ATTACK` addon is active.

## Combat Readiness Score
- `ComputeFulfillmentLevel` inspects each frontier military building (`FrontierDistance::Near`) that is not new. Inland outposts are ignored.
- For each qualifying building it asks for available attackers via `nobMilitary::GetSoldiersForAttack`. The first (strongest) soldier is skipped to keep at least one defender home.
- Remaining candidates contribute a weighted score (`kSoldierAttackWeights = {3,4,5,6,7}` mapped by rank). The function returns the average weight per active frontier building, with `GetCombatAttackWeight` exposing the raw sum. This captures both the number of ready attackers and their quality.

## Switching Between Defense and Attack
- `UpdateCombatMode` keeps `attackMode` in either `DefenseMode` or `AttackMode`, using the configured fulfillment thresholds:
  - Fulfillment ≥ high forces `AttackMode`.
  - Fulfillment ≤ low forces `DefenseMode`.
  - Between low/medium or medium/high, cosine-weighted randomness prevents oscillation: medium fulfillment nudges defense into attack, low-to-medium fulfillment can pull attack back to defense.
- The current mode gates ground assaults: defense mode is conservative, while attack mode allows any qualifying target.

## Ground Assault Selection (`TryToAttack`)
- The routine scans up to ~40 frontline military buildings (skipping extras randomly when the roster is large) and gathers enemy military structures within a pathfinding radius of 2 tiles.
- Targets must be visible, belong to an attackable player, and lie within `BASE_ATTACKING_DISTANCE`. Unmanned headquarters or harbors are queued first for priority strikes; the rest of the list is shuffled for variety.
- For each candidate target:
  - Nearby friendly military buildings (radius 2) offer troops unless under attack. `GetSoldiersStrengthForAttack` returns both the soldier count and combined combat strength.
  - On Hard difficulty, the AI skips enemy forts if the attackers’ strength does not exceed the defenders’ strength by at least 3, or if the enemy has no stationed troops.
  - In defense mode, `CanAttackInDefenseMode` further restricts actions to “lonely” enemy strongholds (no other enemy forts within radius 12) and only if the attacker count strictly exceeds the defenders.
- Once a target passes all checks, the AI issues `aii.Attack(targetPos, attackersCount, true)` and stops further evaluation that frame.

## Sea Assaults (`TrySeaAttack`)
- Preconditions: at least one ship, at least one harbor, and the `SEA_ATTACK` addon enabled.
- The AI groups ships by sea ID and records where loaded troops are available; sea zones without embarkable soldiers are discarded.
- It first inspects visible harbors: undefended enemy harbors are enqueued with highest priority, while allied or invisible harbors trigger a broader search for neighboring military targets.
- For remaining harbor points (occupied by allies or empty), it scans surrounding military sites, queuing undefended HQ/harbors separately from regular targets.
- Both undefended and regular lists are shuffled; the AI always tries undefended options first. Before committing it verifies that suitable ships can reach the target via `GetSoldiersForSeaAttack`.
- Successful checks result in `aii.SeaAttack(targetPos, numAttackers, true)`; otherwise the AI keeps searching until lists are exhausted.

## Defensive Biases
- Defense mode permits counterattacks only against isolated enemy strongholds where the AI can outnumber the garrison, preventing overextension when troop readiness is low.
- Frontier-only scoring and the deliberate omission of the strongest soldier per garrison ensure border posts retain a minimum defense even when the AI feels ready to strike.
