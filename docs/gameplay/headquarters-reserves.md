# Headquarters Reserve Soldiers

The HQ inherits the soldier reserve system from `libs/s25main/buildings/nobBaseWarehouse.*`.
Three parallel arrays track reserve state for each rank (`NUM_SOLDIER_RANKS`):

- `reserve_soldiers_claimed_real`: authoritative target per rank.
- `reserve_soldiers_claimed_visual`: UI mirror of the target so remote clients/replays can stay in sync.
- `reserve_soldiers_available`: actual soldiers currently parked in the reserve pool.

## Initialization

During `nobHQ` construction the game loads the configured start wares, copies the visual inventory into the real counters,
and merges the stock into the global inventory snapshot. Afterwards every military rank up to
`world->GetGGS().GetMaxMilitaryRank()` is configured with a reserve target of one soldier and `RefreshReserve`
is called to pull those soldiers out of the active inventory (`libs/s25main/buildings/nobHQ.cpp`).

## Reserve Enforcement (`RefreshReserve`)

`RefreshReserve` compares `reserve_soldiers_available[rank]` against the target for that rank:

- If too few reserves are available, it transfers up to the needed amount from `inventory` into the reserve buffer.
- If too many reserves are held, it releases the surplus back into `inventory`, triggers `CheckOuthousing`
  so “send away” settings apply, and calls `world->GetPlayer(player).RegulateAllTroops()` to rebalance other
  garrisons.

Both `SetRealReserve` (authoritative changes) and soldier arrival/return paths call `RefreshReserve`
to keep the warehouse state consistent. Remote clients or replays mirror authoritative updates into the visual counts.

## Drawing Flags

The HQ draw code shows up to four flags based on the sum of currently available reserve soldiers plus active soldiers
(`GetNumSoldiers()`). This is evaluated every frame before rendering (`libs/s25main/buildings/nobHQ.cpp`).

## Adjusting Reserves from the UI

The HQ window (`libs/s25main/ingameWindows/iwHQ.cpp`) exposes a “Reserve” page that renders one row per soldier rank.
Each row contains `-`/`+` buttons and a `VarText` label that shows `available/claimed` using pointers returned by
`GetReserveAvailablePointer` and `GetReserveClaimedVisualPointer`. When a button is pressed the window computes the new
requested count and calls `GAMECLIENT.ChangeReserve`. The `GameCommandFactory` wraps this request into
`gc::ChangeReserve`, and on execution (`libs/s25main/GameCommands.cpp`) the command invokes
`nobBaseWarehouse::SetRealReserve(rank, count)` on the targeted building.

`SetRealReserve` updates the authoritative target, mirrors it to the visual state for remote clients or replays, and
finally calls `RefreshReserve` so the reserve pool matches the requested number. The UI meanwhile optimistically calls
`SetReserveVisual` so the label reflects the new value immediately while the command is in flight.

## Interaction with Defenders

When the HQ needs to provide a defender it first tries to use an actively garrisoned soldier.
If none of the desired rank are active it consumes one from `reserve_soldiers_available`, re-adding it to the
visual inventory so UI counters stay non-negative. Returning soldiers also pass through `AddActiveSoldier`,
which adds them back into inventory and immediately refreshes the reserve count before other buildings retune their troop distribution
(`libs/s25main/buildings/nobBaseWarehouse.cpp`).

## Building Destruction

If the HQ (or any warehouse) gets destroyed the constructor logic moves every soldier that was still in
`reserve_soldiers_available` back into the real inventory so the fleeing-population handling can
spawn them correctly (`libs/s25main/buildings/nobBaseWarehouse.cpp`).
