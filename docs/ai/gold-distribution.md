# AI Gold Distribution

This note describes how the current JH AI gets gold coins into military
buildings.

The important distinction is:

- the AI does not directly assign a numeric gold budget to each military
  building
- the AI mostly decides which buildings may receive coins at all
- the economy code then routes concrete coin deliveries to the best eligible
  destination

See also:

- `docs/gameplay/military/nob-military.md`
- `docs/ai/troops-limiting.md`

## Overview

For military upgrades, coin flow has three layers:

1. `nobMilitary` decides whether an individual building wants coins and can
   order another one.
2. `GamePlayer` decides which military building should receive a free coin
   already present in the economy.
3. `AIPlayerJH` decides which military buildings should have coin delivery
   enabled or disabled.

In the current codebase, layer 3 is fairly simple and layer 2 does most of the
real distribution work.

## Building-Level Demand

`nobMilitary::SetCoinsAllowed(bool enabled)` is the local gate for a military
building.

- Enabling coin supply clears `coinsDisabled` and immediately calls
  `SearchCoins()`.
- Disabling coin supply cancels only ordered coins that are still outside the
  building. Coins already stored inside remain there, and coins already being
  carried in are not recalled.

The building only wants coins if `WantCoins()` returns true. That requires all
of the following:

- coin supply is enabled for the building
- stored plus ordered coins are below the building's coin capacity
- the building is no longer `new_built`
- at least one stationed soldier is still upgradeable

When the building wants coins, `SearchCoins()` orders exactly one coin from a
warehouse and schedules a later re-check. This makes coin ordering incremental
instead of bulk-based.

When a coin arrives:

- `AddWare()` increments `numCoins`
- `PrepareUpgrading()` schedules a promotion event if possible
- the promotion event upgrades one lowest-ranked soldier and consumes one coin
- after the upgrade, the building calls `SearchCoins()` again if it still needs
  more

So a military building effectively pulls coins one by one as long as it has
upgradeable soldiers and coin delivery stays enabled.

## Coin Priority Score

When the economy needs to choose among military buildings, it uses
`nobMilitary::CalcCoinsPoints()`.

The current formula is:

```text
points = 10000
points -= 30 * (stored_coins + ordered_coins)
points += 20 * upgradeable_soldiers
```

If `WantCoins()` is false, the score is `0`.

Implications of this scoring:

- buildings that cannot use coins are removed from consideration entirely
- buildings already holding or expecting coins become less attractive
- buildings with more soldiers below max rank become more attractive

The score does not include frontier distance directly. Frontier relevance
usually enters earlier through the AI's enable/disable decisions.

## Economy-Side Distribution

The concrete destination choice for a coin happens in
`GamePlayer::FindClientForCoin(const Ware&)`.

The algorithm is:

1. Iterate over all owned military buildings.
2. Ask each one for `CalcCoinsPoints()`.
3. Skip buildings with score `0`.
4. Check whether a ware path exists to that building.
5. Subtract path length from the score.
6. Pick the building with the highest remaining score.

In shorthand:

```text
effective_score = CalcCoinsPoints() - path_length
```

This means coin distribution among eligible military buildings is driven by a
combination of:

- upgrade demand
- already reserved or stored coins
- road reachability
- transport distance

If no military building qualifies, the ware falls back to warehouse logic.

## Two Coin Paths

Coins can reach military buildings through two related mechanisms.

### Direct military-building orders

`nobMilitary::SearchCoins()` can directly ask `GamePlayer::OrderWare(Coins,
this)` for one coin from a warehouse. This is the most explicit military coin
request path.

### Generic ware retargeting

Coins that already exist in the economy can also be routed through the generic
ware-selection path:

- `GamePlayer::FindClientForWare()` special-cases `GoodType::Coins`
- for coins, it delegates to `FindClientForCoin()`

That path is used when a coin is created or released without already having a
fixed destination.

## What The AI Actually Controls

The JH AI mostly influences military gold distribution by sending
`SetCoinsAllowed(...)` commands for individual buildings.

The active decisions in the current code are:

- `AIEventHandler::HandleNewMilitaryBuildingOccupied()` enables coins for
  newly occupied non-`Far` military buildings
- the same handler disables coins for some `Far` inland small buildings
- `AIEventHandler::HandleBorderChanged()` re-enables coins when a military
  building becomes non-`Far`

In practice, this means the AI usually keeps coin supply enabled on military
buildings that matter for border pressure and disabled on some deeper inland
positions.

## Legacy Upgrade-Building Logic

There is older AI code in `AIMilitaryLogistics::MilUpgradeOptim()` that tries
to shape coin usage and troop composition around a chosen "upgrade building".

Two details matter for current behaviour:

- the periodic call to `MilUpgradeOptim()` in `AIPlayerJH::RunGF()` is
  commented out
- `AIEconomyController::UpdateUpgradeBuilding()` currently returns `-1`

Because of that, the old "focus one upgrade building" policy is effectively
inactive in the shipped runtime path.

So when analyzing current AI matches, assume:

- coin delivery is primarily gated by per-building `SetCoinsAllowed(...)`
- actual distribution among enabled buildings is mostly handled by
  `FindClientForCoin()`

## Practical Summary

The current AI does not do fine-grained gold balancing itself.

Instead it uses a two-step policy:

1. mark some military buildings as coin-enabled and others as coin-disabled
2. let the normal economy route coins to the enabled military building with the
   best demand-minus-distance score

If you want to change how AI gold is spread across military buildings, the most
important code paths are:

- `libs/s25main/buildings/nobMilitary.cpp`
- `libs/s25main/GamePlayer.cpp`
- `libs/s25main/ai/aijh/runtime/AIEventHandler.cpp`
- `libs/s25main/ai/aijh/runtime/AIMilitaryLogistics.cpp`
