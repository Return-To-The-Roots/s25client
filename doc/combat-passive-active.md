## Combat & Healing Flow Overview

This memo documents the combat lifecycles discovered while tracing `nofPassiveSoldier` through the codebase. Paths and line numbers reference the current tree.

### Passive Soldiers & Healing
- `libs/s25main/figures/nofPassiveSoldier.cpp:16-127` keeps every garrisoned soldier in a heal loop. When an active soldier returns and is cloned into a passive (`nofPassiveSoldier(const nofSoldier&)`), `Heal()` schedules a `healing_event` if `hitpoints < HITPOINTS[GetRank()]`.
- The handler (`HandleDerivedEvent`, lines 54-79) only increments HP while the figure remains in `FigureState::Job`. Each tick uses `CONVALESCE_TIME + rand(CONVALESCE_TIME_RANDOM)` (see `libs/s25main/gameData/MilitaryConsts.h:52-97`) as the delay, guaranteeing gradual recovery up to the per-rank cap.
- Promotions (`Upgrade`, lines 114-127) reset HP to the rank maximum and adjust the owning player’s job inventory. Healing never happens outside a building; passive status is required.

`### From Passive To Active Missions
- Military buildings convert a passive to an active mission unit through `nobMilitary::SendAttacker` (`libs/s25main/buildings/nobMilitary.cpp:723-732`). It clones the passive into a `nofAttacker` (subclass of `nofActiveSoldier`) so the current HP travels with the unit, removes the old passive from `troops`, and tracks the mission soldier in `troops_on_mission`.
- When an active returns, `nobMilitary::AddActiveSoldier` (`libs/s25main/buildings/nobMilitary.cpp:646-704`) wraps it back into a passive via `std::make_unique<nofPassiveSoldier>(*soldier)`, immediately re‑entering the healing cycle. Attrition taken in the field thus persists until the unit spends time inside a building.

### Active Soldier State Machine
- `nofActiveSoldier` (`libs/s25main/figures/nofActiveSoldier.cpp:200-420`) handles roaming attackers, defenders, and aggressive defenders. `TryFightingNearbyEnemy` scans a radius of two tiles for other actives ready to fight, negotiates a fight spot via `GetFightSpotNear`, and transitions both soldiers to `MeetEnemy`/`WaitingForFight`.
- Movement logic (`MeetingEnemy`) walks toward the shared `fightSpot_`. If either cannot reach it, `AbortFreeFight` resets both soldiers so they can resume mission logic. Damage application happens through `TakeHit()` (lines 343-347), which simply decrements the stored `hitpoints`; there is no regeneration outside of buildings.

### Defender Hitpoint Bonus
- When a passive is promoted into an active defender, `nofDefender::ApplyDefenderBonusHitpoints` (`libs/s25main/figures/nofDefender.cpp:17-77, 132-161`) now grants up to two temporary HP based on ownership history. Defenders get +1 HP if the building has been held for more than 5 000 gameframes since the last capture, and an additional +1 HP if the defender’s player matches `nobBaseMilitary::GetOriginOwner()`. The bonus only applies to soldiers that left the building at full rank HP, is clamped, and disappears once they re-enter the garrison and become passive again via `std::make_unique<nofPassiveSoldier>(*soldier)`.

### Fulfillment Level Calculation
- `AIPlayerJH::ComputeFulfillmentLevel` (`libs/s25main/ai/aijh/AIPlayerJH.cpp:1708-1764`) scores how ready each frontline building is to launch an attack. The AI iterates all military buildings returned by `aii.GetMilitaryBuildings()`, filters down to established ones (`!IsNewBuilt()`) that sit on a near frontier, and skips entries where the flag or attacker list is missing.
- For every qualifying building it pulls the passive soldiers returned by `nobMilitary::GetSoldiersForAttack(flag->GetPos())`, intentionally ignoring the first element (the strongest rank) so that one elite remains garrisoned. The rest are weighted by `kSoldierAttackWeights = {3,4,5,6,7}` based on their current rank and summed into `totalWeight`.
- The routine counts only the buildings that passed all filters (`frontierCount`) and returns the average weight (`totalWeight / frontierCount`). `outTotalWeight`, when provided, exposes the raw pool the AI could commit across all frontlines and is later compared against randomly rolled probabilities before deciding to switch into combat mode or stay defensive.

### Combat Mode Switching
- `AIPlayerJH::ComputeEnemyFrontlineWeight` (`libs/s25main/ai/aijh/AIPlayerJH.cpp:1780-1826`) mirrors the fulfillment logic for every attackable opponent. It inspects their `FrontierDistance::Near` military buildings, ignores newly built outposts, drops the strongest available soldier per site, and sums the remaining weights with the same rank table so both sides are measured consistently.
- `AIPlayerJH::UpdateCombatMode` (`libs/s25main/ai/aijh/AIPlayerJH.cpp:1838-1890`) now performs a superiority check before the older probabilistic thresholds. If our total frontline weight exceeds three times the computed enemy weight, the AI immediately flips to `CombatMode::AttackMode`. Otherwise it falls back to the configured fulfillment bands, which still use cosine-shaped probabilities between medium and high thresholds when ramping up or down.

### Duel Resolution (`noFighting`)
- When two actives meet successfully, the engine instantiates `noFighting` (`libs/s25main/nodeObjs/noFighting.cpp:22-305`). The constructor removes both soldiers from the map, pauses other traffic at that node, and keeps the area visible for both owners.
- `StartAttack` (lines 275-305) drives the turn system. Every 15 ticks it rolls per-soldier random values weighted by rank and the `ADJUST_MILITARY_STRENGTH` addon. Higher rolls mean the attacker lands a hit (`defending_animation == 3`); otherwise, the defender plays one of three block animations.
- The event loop (`HandleEvent`, lines 181-269) applies hits by calling `TakeHit()` on the defender. Reaching zero HP triggers `LostFighting()` on the loser, re-adds the surviving soldier to the map (still carrying the remaining HP), schedules a death animation, drops a skeleton if the tile was empty, and finally destroys the dead figure while updating statistics and visibility.

### End-To-End Flow
1. Passive soldiers heal inside buildings via scheduled events until their per-rank HP cap.
2. Converting to an active attacker/defender copies the current HP; no healing occurs in the field.
3. Meeting an enemy spawns a `noFighting` object that alternates attack turns until one soldier’s HP hits zero.
4. Survivors resume their mission with the reduced HP. Upon re-entering a friendly military building, they become passives again and restart the healing timer.

This chain ties HP loss directly to duel outcomes and recovery to time spent safely garrisoned.
