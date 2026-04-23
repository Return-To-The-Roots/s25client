# AI Resource Value Cache

See also:

- [position-finding.md](position-finding.md) — primary caller of
  `CalcResourceValue` via the global building-position search.
- [performance-profiling.md](performance-profiling.md) — where the
  cache hit/miss counters mentioned below surface alongside section
  timings in `ai_performance.csv`.

`AIQueryService::CalcResourceValue(pt, res)` sums `GetResourceRating` across all
map points within the resource radius. This is the dominant inner-loop cost during
global building-position searches and border-slot reservation checks. The same
`(pt, res)` pair is frequently queried multiple times within a single game frame by
independent callers, so results are cached inside `AIQueryService`.

## What is cached

Only the **non-directional** form of `CalcResourceValue` is cached — the form where
`direction == boost::none`. This is the full-radius sum used by all AI planning code.

The **directional incremental** form (`direction + lastval`) is an optimization used
by `AIResourceMap` to roll the radius window one step at a time. Its return value
depends on `lastval`, which is not part of the cache key, so those calls bypass the
cache entirely and always recompute.

## Cache key and storage

```
key:   (MapPoint pt, AIResource res)
value: (int value, unsigned expiresAtGF)
```

Entries are stored in an `unordered_map` keyed by a `(MapPoint, AIResource)` pair.
Each entry records the computed value and the **absolute game frame at which it
expires** (`currentGF + TTL`). Validity is checked as `currentGF < expiresAtGF`.

## Per-resource TTL policy

Different resource types have different expiration windows, reflecting how quickly
each resource type can change in the game world:

| Resource | Condition | TTL (game frames) | Rationale |
|----------|-----------|-------------------|-----------|
| `Fish` | any | 10,000 | Fish deplete slowly; worth tracking medium-term trends |
| `Gold`, `Ironore`, `Coal`, `Granite`, `Stones` | value == 0 | 1,000,000 | No deposit exists; cannot appear where absent |
| `Gold`, `Ironore`, `Coal`, `Granite`, `Stones` | value > 0 | 1,000 | Deposits deplete as mines or quarries work |
| `Wood` | any | 1,000 | Trees are actively chopped and planted |
| `Plantspace` | any | 1,000 | Changes on farm or forester placement |
| `Borderland` | any | 30,000 | Invalidated on territory changes; TTL is a safety net |

The zero-value permanence for subsurface resources (`Gold`, `Ironore`, `Coal`,
`Granite`) and `Stones` is correct by the game model: deposits are fixed at map
generation and cannot appear where none existed. A zero rating for these resources
is therefore safe to treat as permanent.

At 25 game frames per second, 1,000 frames ≈ 40 s and 10,000 frames ≈ 7 min.
The permanent TTL of 1,000,000 frames ≈ 11 h of continuous play; it is effectively
permanent for any realistic session.

## Eviction

Eviction is lazy and amortized — it runs inside `MaybeEvictExpired`, called on
every cache miss. The sweep is skipped unless one of two conditions is met:

- The miss counter has reached `kEvictEveryNMisses` (4096) since the last sweep, or
- The cache map exceeds the hard-cap `kResourceValueCacheHardCap` (200,000 entries).

During a sweep all entries with `currentGF >= expiresAtGF` are erased. If the map
still exceeds the hard cap after the sweep it is cleared entirely. This prevents
unbounded growth on very large maps where a single global search can touch hundreds
of thousands of distinct points.

## Hit/miss statistics

`AIQueryService` tracks cumulative hits and misses:

```cpp
unsigned long long GetResourceValueCacheHits()   const;
unsigned long long GetResourceValueCacheMisses() const;
void               ResetResourceValueCacheStats();
```

These counters are used to verify cache effectiveness and can be wired into the AI
profiler output alongside the construction-job timings in `ai_performance.csv`.

## Targeted `Borderland` invalidation

`CalcResourceValue(pt, AIResource::Borderland)` depends on `IsBorder` and
`IsOwnTerritory`, which flip whenever a military building finishes, is destroyed,
or is captured. Rather than relying solely on the TTL, each `AIPlayerJH` subscribes
to `NodeNote::Owner` and calls `InvalidateResourceValueInRadius(changed_pt,
Borderland, RES_RADIUS[Borderland] + 1)` for every owner-flipped point. The
`+1` covers `IsBorder` propagation one step beyond the changed point.

This makes the 30,000-frame TTL a backstop only; actual freshness is maintained
by the invalidation hook so `Borderland` entries stay valid during steady-state
play and are evicted precisely on territory transitions.

## Source files

| File | Role |
|------|------|
| `libs/s25main/ai/AIQueryService.h` | `CachedResourceValue`, `CacheKey`, `CacheKeyHash` structs; stat accessors; private declarations |
| `libs/s25main/ai/AIQueryService.cpp` | `GetCacheTTL`, cache lookup/insert in `CalcResourceValue`, `MaybeEvictExpired` |
