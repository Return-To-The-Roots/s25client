# AI Adjustment Synopsis

See also:

- [configuration.md](configuration.md) — `toolPriority` config keys feed
  `calcToolPriority` defaults.
- [statistics-logging.md](statistics-logging.md) — `saveStats` is where the
  distribution slider snapshot described below is emitted.

## Tool Priority Logic
- `AIPlayerJH::AdjustSettings` refreshes tool priorities whenever at least one
  Metalworks exists. It first computes shortages with `calcToolPriority`,
  which credits each tool when worker requirements exceed the inventory of
  the tool’s associated good.
- If any shortage is detected, those tools retain their calculated
  `TOOL_PRIORITY` values and other tools remain at zero.
- When no shortages remain, inventories are compared to the
  `TOOL_BASIS` minimums (see `libs/s25main/gameData/ToolConsts.h`). Tools
  below their baseline inherit the matching `TOOL_PRIORITY`, while tools
  meeting the baseline drop to zero.
- If every tool meets or exceeds its baseline, the AI reverts to the default
  `TOOL_PRIORITY` map for the entire tool set before invoking
  `aii.ChangeTools` if differences exist.

## Goods Distribution Logging
- `AIPlayerJH::saveStats` now captures the player’s distribution sliders right
  after the Tools section by calling `player.FillVisualSettings`.
- For each mapping in `distributionMap`, the routine prints the associated
  good and building in the format `Good: Building value,...` (e.g.
  `Iron: Armory 10,Metalworks 8`). This mirrors what players see in the
  distribution UI and documents the exact priorities feeding each industry at
  the time the stats snapshot is written.
