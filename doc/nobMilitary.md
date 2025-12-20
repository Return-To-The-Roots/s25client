# Military Building Logic (`nobMilitary.cpp`)

## Overview
`nobMilitary` models garrisoned military buildings. It derives from `nobBaseMilitary` and extends it with garrison management, coin-driven promotions, capture handling, and frontier awareness. The class is instantiated for each finished military building and persists across save/load cycles.

## Core State and Initialization
- During construction the building registers itself in the world military grid, determines its size from the building type, presets troop limits to the maximum capacity, and keeps the entrance open until the first soldier arrives.
- Add-ons (`NO_COINS_DEFAULT`, `FRONTIER_DISTANCE_REACHABLE`, `SEA_ATTACK`, etc.) modify runtime behaviour, e.g. disabling coins by default or affecting frontier flag calculation.
- Serialization persists flags such as `new_built`, coin counts, frontier distance, queued events, soldier containers, and per-rank troop limits.

## Frontier Distance Tracking
- `LookForEnemyBuildings` scans nearby military buildings to classify the building as `Far`, `Mid`, `Near`, or `Harbor`. The result controls troop requirements and UI flags.
- Distances consider both radius overlap and extended attack range. Optional pathfinding ensures only reachable enemies tighten the frontier status.
- Allies adjust their own frontier state via `NewEnemyMilitaryBuilding`, and harbors may push the state to `Harbor` when sea attacks are possible.

## Troop Regulation Loop
- `RegulateTroops` centralises garrison balancing. It tallies stationed, ordered, mission, and defender soldiers against the per-rank limits.
- Excess soldiers are cancelled (preferring those still en route). Shortages trigger orders through `GamePlayer::OrderTroops`, provided road access exists and add-on restrictions allow it.
- The method avoids re-entrancy, suspends regulation while the building is being captured, and reacts to events like attackers leaving or coins being consumed.
- `CalcRequiredNumTroops` converts frontier status and player military settings into target garrison sizes.

## Coin Supply and Promotion
- Gold coins are ordered from warehouses when `WantCoins` indicates capacity, demand (upgradeable soldier), and coin policy alignment.
- `SearchCoins` schedules coin deliveries and retries via event callbacks. `TakeWare`, `AddWare`, and `WareLost` track in-flight coins and adjust counts.
- `PrepareUpgrading` schedules promotion events when coins and promotable soldiers are available. `HandleEvent` case `2` consumes a coin to upgrade the weakest-ranked soldier and restarts regulation afterwards.

## Soldier Lifecycle
- `AddPassiveSoldier` inserts arriving troops, triggers territory expansion on first occupation, and posts notifications.
- `AddActiveSoldier` handles returning attackers/defenders: they are converted back to passives, reassigned, or destroyed if capture state requires it.
- `SoldierLost`, `CancelOrders`, and `GotWorker` keep the ordered lists in sync when soldiers fail to arrive or are reassigned.
- `ChooseSoldier` respects player military settings when selecting attackers or defenders. `GetNumSoldiersForAttack` and strength helpers evaluate outgoing attack capacity.

## Combat, Capture, and Destruction
- `ProvideDefender` supplies defenders on demand, falling back to cancelling missions if the garrison is empty.
- `nobBaseMilitary` records both the capture gameframe (`captured_gf_`) and the building’s original owner (`origin_owner_`). On construction the owner is set to the founding player; subsequent captures rewrite only `captured_gf_`, letting the engine distinguish a structure’s “age” under the current owner while still knowing who established it. The pair is serialized with the base state so multiplayer replays and savegames retain that provenance.
- `Capture` transfers ownership: inventory coins are reassigned, missions cancelled, flags captured, territory recalculated, allies notified, and add-on coin policies applied.
- `NeedOccupyingTroops` and `FarAwayCapturerReachedGoal` control how attackers step into a building during capture, including far-away capturers when nearby soldiers are scarce.
- `CapturingSoldierArrived` finalises capture once enough attackers enter. `UnlinkAggressor` and `IsAggressor/IsFarAwayCapturer` management ensure aggressor lists stay clean.
- `DestroyBuilding` removes the building from military grids, sends troops out, cleans pending events, refunds coins, recalc territory, and posts notifications.
- `HitOfCatapultStone` simulates catapult damage by removing one garrisoned soldier and re-triggering regulation.

## Losing Territory and Collateral Cleanup
- `nobMilitary::DestroyBuilding()` (`libs/s25main/buildings/nobMilitary.cpp`) removes the structure from the military grid, cancels troop/coin events, ejects garrisoned soldiers as wanderers, notifies pending capturers/defenders, refunds coins, and then calls `nobBaseMilitary::DestroyBuilding()` so mission soldiers/aggressors are told their home vanished. If the building was occupied (`!new_built`), the base call is followed by `GameWorld::RecalcTerritory(..., TerritoryChangeReason::Destroyed)` to shrink the owner’s land.
- `nobMilitary::Capture()` performs a similar cleanup for the losing owner (cancel missions, notify aggressors, transfer coins) but then swaps the building’s player, captures its flag, and recalculates territory with reason `Captured`. Visibility is reduced for the loser, nearby allied/enemy garrisons recompute frontier flags, and both players receive `BuildingNote::Captured/Lost` notifications.
- `GameWorld::RecalcTerritory()` (`libs/s25main/world/GameWorld.cpp`) builds a `TerritoryRegion` around the trigger, copies new ownership back to the map, and gathers the affected nodes plus their neighbors. Each node runs through `DestroyPlayerRests`, which tears down enemy flags/buildings/building sites now standing on foreign ground (HQs, harbors, and already-occupied military buildings are exempt). Flags that fall will destroy their attached buildings, and captures feed `CombatLossTracker` so the attacker gets credit for collateral.
- After structural cleanup, RecalcTerritory also destroys orphaned road segments that cross non-owned or border tiles, adjusts minimap/BQ/border stones, updates visibility, and fires scripting + player notifications about the new frontier.
- AI player `AIPlayerJH` listens to `BuildingNote::Lost` events: `HandleMilitaryBuilingLost()` records the coordinates as “recently lost” (used later by attack heuristics) and reuses `HandleLostLand()` to prune roads from the lost area once at least one storehouse remains (`libs/s25main/ai/aijh/AIPlayerJH.cpp:1312-1541`).

## Risk & Importance Snapshots
- Every 500 gameframes `GameWorld::UpdateMilitaryRiskEstimates()` (`libs/s25main/Game.cpp`, `libs/s25main/world/GameWorld.cpp`) scans all occupied `nobMilitary` instances. Buildings currently under attack keep their last snapshot so the UI does not oscillate mid-fight.
- The routine calls `GameWorldBase::ComputeCaptureRisk()` to evaluate the same attacker/defender strength ratio the AI uses (`libs/s25main/world/GameWorldBase.cpp`). The result is clamped to `[0,1]` and stored through `nobMilitary::SetCaptureRiskEstimate`.
- Importance is stored as the number of dependent buildings that `DestroyPlayerRests` would remove if the garrison were captured (`nobMilitary::EstimateCaptureLossCount`). Those counts are cached on the building so `GameWorldView::DrawProductivity` (`libs/s25main/world/GameWorldView.cpp`) simply prints `[capt:0.32,imp:4.00]` under the soldier count.
- AIPlayerJH’s internal `EvaluateCaptureRisks` now just delegates to the world helper, so AI stats, combat logs, and the HUD all read the same refresh cycle without triggering extra calculations inside the render loop.

## Capture-Risk Estimation
- Every `nobMilitary` caches the probability that nearby enemies could capture it. The value is derived from
  `GetGarrisonStrengthWithBonus()`, which includes temporary defender hitpoint bonuses, versus the total attacking
  strength enemies could send (`GetSoldiersStrengthForAttack`).
- `AIPlayerJH` refreshes the risk on the same 500-gameframe cadence it uses for `UpdateCombatMode()`, updating frontline
  buildings just before reconsidering combat posture. The snapshot is stored inside the building, exposed via
  `GetCaptureRiskEstimate()`, and serialized into combat logs so analysts can correlate attack outcomes with prior risk.

## Event Handling
- Event IDs drive exit sequencing (0), coin rechecks (1), and soldier promotion (2). The exit loop drains `leave_house`, schedules follow-up exits, and invokes troop regulation when movement settles.
- Additional timer-driven behaviour originates from `SearchCoins` and `PrepareUpgrading`, which register events with the global `EventManager`.

## Demolition and Utility Checks
- `IsDemolitionAllowed` enforces add-on rules that block demolition if the building is under attack or near the frontier.
- `IsUseless` and `IsAttackable` determine higher-level AI or UI behaviours based on frontier status, capture state, and ownership.
- Helper queries like `GetTotalSoldiersByRank`, `HasUpgradeableSoldier`, `HasMaxRankSoldier`, and `GetSoldiersStrength` expose garrison state to other systems.
