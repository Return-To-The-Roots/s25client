# Data Extractor Synopsis

`libs/s25main/dataextractor/DataExtractor.cpp` implements a lightweight snapshot
writer for tooling that inspects in-game state outside the normal renderer.
It runs next to the `GamePlayer` object and captures a per-frame view of the
player's economy, then prints the values either as CSV or JSON via stdout.

## Snapshot generation

- `ProcessSnapshot(GamePlayer&, uint32_t gameframe)` is the single entry point
  for collecting data. It first records per-frame metadata (`"GameFrame"` and
  the owning `"PlayerId"`), then builds a `SnapshotData` map keyed by
  descriptive strings for statistics or translated building/ware names sourced
  from `BUILDING_NAMES_1` / `GOOD_NAMES_1`.
- Statistics: iterates across every `StatisticType` and stores the player's
  current value under the readable name returned by `StatisticTypeName`.
- Buildings: queries the building register for each `BuildingType`, storing
  constructed count, open sites, and average productivity (`FooCount`,
  `FooSites`, `FooProd`).
- Inventory: grabs the current `Inventory` from the player and records every
  `GoodType` amount.
- The method tracks the field names in a `std::set` so later CSV exports have
  deterministic headers with `GameFrame` first and `PlayerId` immediately
  after it.

Only the latest snapshot is retained (`currentSnapshot_`), so callers should
flush or otherwise consume the data before requesting another frame.

## Output formats

- `SerializeCsv(const SnapshotData&, std::ostream&, bool write_header)` prints
  a CSV representation of the snapshot, optionally writing the header row
  (defaults to `GameFrame`, `PlayerId`, then every other field in sorted
  order). Values missing from the snapshot default to `0`.
- `SerializeJson(const SnapshotData&, std::ostream&)` writes the snapshot as a
  single compact JSON object followed by a newline.
- The `extras/data-extractor/main.cpp` tool calls these helpers, forwards the
  resulting text to `stdout`, logs success or errors, and clears the cached
  snapshot via `ClearSnapshot()` before processing the next player.

In practice a caller invokes `ProcessSnapshot` at interesting frames, then
chooses a serializer (CSV/JSON) to emit the structured data for plotting or
post-processing without touching the core simulation code. The standalone
`extras/data-extractor` utility provides a reference implementation.
