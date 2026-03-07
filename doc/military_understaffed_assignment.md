# Understaffed Military Garrison Assignment

This document describes how the engine assigns soldiers when total army size is lower than the sum of all desired
garrisons.

## 1. Understaffed detection

Each military building runs `nobMilitary::RegulateTroops()`:

- It computes current total soldiers (`stationed + ordered + on mission + defender/capturers`).
- It computes desired garrison from frontier distance and military settings via `CalcRequiredNumTroops()`.
- It calculates `diff = desired - current`.

If `diff > 0`, the building is understaffed and requests reinforcements.

Reference: `libs/s25main/buildings/nobMilitary.cpp:421`, `libs/s25main/buildings/nobMilitary.cpp:488`

## 2. Request conditions

Before ordering, the building checks there is at least one road connection from its flag (excluding the building
connection edge). If no route exists, no soldiers are ordered.

When under attack and Defender Behaviour add-on mode is `2`, the requested amount is reduced by defender-setting ratio.

Reference: `libs/s25main/buildings/nobMilitary.cpp:554`, `libs/s25main/buildings/nobMilitary.cpp:561`

## 3. Player ordering logic

`GamePlayer::OrderTroops(goal, lack, diff)` performs repeated nearest-warehouse selection:

1. Build rank mask for still-missing ranks.
2. `FindWarehouse(..., FW::HasAnyMatchingSoldier, ...)` picks nearest reachable warehouse with matching soldier rank.
3. Warehouse dispatches soldiers.
4. Repeat until shortage is filled or no matching warehouse remains.

This is greedy and local per request; there is no global optimization across all military buildings.

Reference: `libs/s25main/GamePlayer.cpp:1248`, `libs/s25main/GamePlayer.cpp:1260`

## 4. Warehouse dispatch

`nobBaseWarehouse::OrderTroops(...)` sends soldiers by rank until `max` (remaining requested total) reaches zero or
warehouse stock is exhausted.

Rank order depends on military setting #1:

- high setting: stronger ranks first
- low setting: weaker ranks first

Reference: `libs/s25main/buildings/nobBaseWarehouse.cpp:878`, `libs/s25main/buildings/nobBaseWarehouse.cpp:883`

## 5. Why shortages persist

If global available soldiers are insufficient, unresolved `diff` remains in some buildings. They stay understaffed
until new soldiers are created or returned.

New soldier availability triggers `GamePlayer::NewSoldiersAvailable(...)`, which re-runs regulation in fixed priority:

1. newly built/unoccupied military buildings
2. frontier-near buildings
3. all remaining military buildings

Within each group, processing follows `BuildingRegister` list insertion order (`std::list` + `push_back`).

Reference: `libs/s25main/GamePlayer.cpp:1284`, `libs/s25main/BuildingRegister.cpp:79`

## 6. Arrival lifecycle

- Ordered soldier is registered immediately in target building `ordered_troops` (`GotWorker`).
- On arrival to target, it becomes stationed via `AddPassiveSoldier`.
- If soldier is lost in transit, target removes it from tracking and re-runs `RegulateTroops()`.

Reference: `libs/s25main/buildings/nobMilitary.cpp:661`, `libs/s25main/figures/nofPassiveSoldier.cpp:119`,
`libs/s25main/buildings/nobMilitary.cpp:749`
