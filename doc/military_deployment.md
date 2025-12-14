# Military Building Troop Deployment

Regular garrisons (`nobMilitary`) continuously rebalance their soldiers through the **regulation loop** described in
`libs/s25main/buildings/nobMilitary.cpp`. The goal is to meet the currently desired garrison size and any per-rank limits
that stem from the Military Control add-on.

## Regulation Loop (`nobMilitary::RegulateTroops`)

1. The building aborts if it is under capture or the method is already running (`is_regulating_troops` guard).
2. It gathers the total number of soldiers per rank across:
   - Stationed troops (`troops`)
   - Soldiers ordered but still en route (`ordered_troops`)
   - Attackers/defenders currently on a mission (`troops_on_mission`, `defender_`)
   - Far-away capturers
3. For each rank it compares the count against `troop_limits[rank]`:
   - If there is **excess**, it cancels outstanding orders of that rank and, if a warehouse path exists, ejects the extra
     stationed soldiers (keeping at least one inside). The order of removal depends on the player's second military
     setting, which controls whether weak or strong soldiers leave first.
   - If there is a **shortage**, it records the missing amount. These “lack” values later drive ordering.
4. The method then compares the total headcount against the target returned from `CalcRequiredNumTroops`. This target uses
   the building’s `frontier_distance` (Near/Mid/Far/Harbor) and the corresponding player military setting slot to scale
   between one soldier and the structure’s maximum capacity.
   - Too many soldiers → cancel outstanding orders and eject stationed soldiers as described above.
   - Too few soldiers → ensure there is at least one free road connection from the flag and call
     `GamePlayer::OrderTroops(this, lack, diff)`. When the Defender Behaviour add-on is set to mode 2 and the building is
     under attack, the deficit is clamped by the defender-setting slider before ordering, so reinforcements respect the
     configured proportions.

The loop is triggered from multiple events:

- `nobMilitary::LookForEnemyBuildings`, `SetTroopLimit`, coin promotions, attacker/defender lifecycle events, capture
  completion, etc.
- `GamePlayer::RegulateAllTroops` iterates every military building and calls `RegulateTroops`, typically after soldiers
  return home or reserve changes occur.
- `GamePlayer::NewSoldiersAvailable` tries to satisfy newly recruited soldiers by calling `RegulateTroops` on buildings
  with open demand (first newly built structures, then frontier ones).

## Ordering Soldiers (`GamePlayer::OrderTroops` → `nobBaseWarehouse::OrderTroops`)

`GamePlayer::OrderTroops` receives the required per-rank counts (`lack`) and total maximum deficit (`diff`). It repeatedly:

1. Builds a bitmask of ranks that are still needed.
2. Uses `FindWarehouse` with `FW::HasAnyMatchingSoldier` to locate the nearest warehouse/HQ/harbor that holds the
   requested ranks.
3. Calls `nobBaseWarehouse::OrderTroops(goal, counts, total_max)` on that warehouse.

The warehouse first respects the player’s military setting #1 to pick whether strong or weak soldiers leave first. For
each rank it:

- Sends available `nofPassiveSoldier` units straight from the active inventory via `AddLeavingFigure`.
- If none are active, it pulls from `reserve_soldiers_available`, which may in turn trigger `RefreshReserve`.

Ordered soldiers travel to the target building. Upon arrival `nobMilitary::GotWorker` places them into `ordered_troops`
until they call `AddPassiveSoldier`, at which point they become part of the stationed list, possibly occupying the
building for the first time and expanding territory.

If a soldier cannot reach the destination, `nobMilitary::SoldierLost` removes it from `ordered_troops` (or the mission
list) and immediately re-runs `RegulateTroops`, which can re-order replacements.

## Deployment Behaviour Influences

- **Military settings**:
  - Setting #1 (Strength preference) determines whether weak or strong soldiers are dispatched and sent home first.
  - Setting #2 (Defender share) limits reinforcements when the Defender Behaviour add-on is in mode 2.
  - Settings #4–#7 control the desired garrison sizes for Far/Mid/Near/Harbor frontier states.
- **Frontier distance**:
  - `LookForEnemyBuildings` scans the vicinity and updates `frontier_distance`, which feeds both the UI flag icons and
    `CalcRequiredNumTroops`.
  - Allies also receive updates via `NewEnemyMilitaryBuilding`.
- **Add-ons**:
  - Military Control allows lowering `troop_limits` per rank through the UI or capping the entire garrison size. The
    per-rank controls call `SetTroopLimit` while the total cap issues `SetTotalTroopLimit`; both store the new limit and
    call `RegulateTroops` immediately.
  - Defender Behaviour (mode 2) scales requested reinforcements during sieges.
  - No Coins Default disables automatic coin supply for new buildings; coins matter because promotions can alter rank
    distribution and thus regulation outcomes.

Together these pieces ensure military buildings keep just enough soldiers of the desired ranks, pull reinforcements only
when travel paths exist, and gracefully cancel or re-order troops when circumstances change.

## UI Controls

- **Global military window (`libs/s25main/ingameWindows/iwMilitary.cpp`)**
  - Eight `ctrlProgress` sliders map directly to the `MilitarySettings` array: recruiting rate, strength preference,
    defender share, attacker share, and the four frontier tiers (interior → border/harbor). Each slider uses the
    corresponding `MILITARY_SETTINGS_SCALE` entry for min/max.
  - Changing a slider marks the window dirty; when it closes or the user switches context, `TransmitSettings` builds a
    new `MilitarySettings` payload and calls `GameCommandFactory::ChangeMilitary`. Successful submissions update
    `GAMECLIENT.visual_settings`, so remote clients mirror the change.
  - The Defender Behaviour add-on can hide the “defenders” slider entirely (mode 1). Help and Default buttons open
    explanatory text or restore `GAMECLIENT.default_settings.military_settings`.

- **Per-building military window (`libs/s25main/ingameWindows/iwMilitaryBuilding.cpp`)**
  - Always available controls: help, demolition, toggle gold deliveries (`SetCoinsAllowed`), “go to” camera jump, and
    “next building” cycling. The soldier/coin indicators render the actual troops (including those queued to leave) and
    the stored coin slots. When the Military Hitpoints add-on is active, health values appear above each soldier icon.
  - **Military Control add-on interactions**
    - Mode 1 adds a single “Send max rank soldiers to a warehouse” button. Pressing it issues two
      `SetTroopLimit` commands to temporarily drop the per-rank limit of the highest-ranked soldiers to zero and then
      restore it, forcing them to leave and be replaced by fresh troops.
    - Mode 2 expands the window height and spawns per-rank +/- buttons plus readouts for every `troop_limits` entry.
      Clicking a button sends `SetTroopLimit(building->GetPos(), rank, new_limit)` through the command factory, which
      executes `nobMilitary::SetTroopLimit` on the server. Additional button 10 sends every soldier home except one by
      clamping rank 0 to one slot and the rest to zero.
    - Mode 3 replaces the per-rank controls with a single pair of +/- buttons for the total garrison cap. Each click
      calls `SetTotalTroopLimit(building->GetPos(), new_limit)` to adjust the aggregate target.
    - These UI callbacks eventually invoke `gc::SetTroopLimit::Execute` or `gc::SetTotalTroopLimit::Execute`, which call
      the corresponding setters and trigger `RegulateTroops`, ensuring the gameplay state immediately reflects the UI
      change.
