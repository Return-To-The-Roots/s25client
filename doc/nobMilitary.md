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

## Event Handling
- Event IDs drive exit sequencing (0), coin rechecks (1), and soldier promotion (2). The exit loop drains `leave_house`, schedules follow-up exits, and invokes troop regulation when movement settles.
- Additional timer-driven behaviour originates from `SearchCoins` and `PrepareUpgrading`, which register events with the global `EventManager`.

## Demolition and Utility Checks
- `IsDemolitionAllowed` enforces add-on rules that block demolition if the building is under attack or near the frontier.
- `IsUseless` and `IsAttackable` determine higher-level AI or UI behaviours based on frontier status, capture state, and ownership.
- Helper queries like `GetTotalSoldiersByRank`, `HasUpgradeableSoldier`, `HasMaxRankSoldier`, and `GetSoldiersStrength` expose garrison state to other systems.
