# Proposal 2 — Memoize Resource Values in `AIQueryService::CalcResourceValue`

## Goal

Avoid repeated `CalcResourceValue(pt, res)` computations by caching results inside `AIQueryService`
with per-resource, value-conditional TTLs. Every caller benefits automatically with no call-site
changes.

## Per-Resource TTL Policy

| Resource | Condition | TTL (game frames) | Rationale |
|----------|-----------|-------------------|-----------|
| `Gold` | value == 0 | 1,000,000 | No deposits → won't appear |
| `Gold` | value > 0 | 1,000 | Deposits deplete as mines work |
| `Ironore` | value == 0 | 1,000,000 | Same as Gold |
| `Ironore` | value > 0 | 1,000 | |
| `Coal` | value == 0 | 1,000,000 | Same as Gold |
| `Coal` | value > 0 | 1,000 | |
| `Granite` | value == 0 | 1,000,000 | Same as Gold |
| `Granite` | value > 0 | 1,000 | |
| `Stones` | value == 0 | 1,000,000 | No surface granite objects → won't appear |
| `Stones` | value > 0 | 1,000 | Quarries deplete stone objects |
| `Fish` | any | 10,000 | Fish replenish slowly; short enough to track depletion trends |
| `Wood` | any | 1,000 | Trees are actively chopped/planted — keep short |
| `Plantspace` | any | 1,000 | Changes on farm/forester placement |
| `Borderland` | any | 1,000 | Changes on territory shifts |

The key insight: a zero value for Gold, Ironore, Coal, Granite, or Stones means no resource exists
at that point, and resources of these types cannot appear where they did not already exist
(deposits are fixed at map generation; quarried stone objects cannot regenerate). So a zero is
effectively permanent and caching it indefinitely is correct.

### Implementation: `GetCacheTTL` helper

```cpp
namespace {
constexpr unsigned kDefaultTTL  = 1'000;
constexpr unsigned kFishTTL     = 10'000;
constexpr unsigned kPermanentTTL = 1'000'000;

unsigned GetCacheTTL(AIResource res, int value)
{
    switch(res)
    {
        case AIResource::Fish:
            return kFishTTL;
        case AIResource::Gold:
        case AIResource::Ironore:
        case AIResource::Coal:
        case AIResource::Granite:
        case AIResource::Stones:
            return (value == 0) ? kPermanentTTL : kDefaultTTL;
        default:  // Wood, Plantspace, Borderland
            return kDefaultTTL;
    }
}
} // namespace
```

## Design

### Cache structures (AIQueryService.h)

```cpp
private:
    struct CachedResourceValue {
        int      value;
        unsigned expiresAtGF;   // absolute game frame at which entry is stale
    };

    struct ResourceCacheKey {
        MapPoint  pt;
        AIResource res;
        bool operator==(const ResourceCacheKey& o) const
        { return pt == o.pt && res == o.res; }
    };
    struct ResourceCacheKeyHash {
        size_t operator()(const ResourceCacheKey& k) const
        {
            // Combine MapPoint (x,y packed as uint32) with resource ordinal.
            const size_t ptHash = std::hash<uint32_t>{}(
                (static_cast<uint32_t>(k.pt.x) << 16) | static_cast<uint32_t>(k.pt.y));
            return ptHash ^ (std::hash<unsigned>{}(static_cast<unsigned>(k.res)) * 2654435761u);
        }
    };

    mutable std::unordered_map<ResourceCacheKey, CachedResourceValue,
                               ResourceCacheKeyHash> resourceValueCache_;
    mutable unsigned long long resourceValueCacheHits_              = 0;
    mutable unsigned long long resourceValueCacheMisses_            = 0;
    mutable unsigned           resourceValueMissesSinceLastEvict_   = 0;
```

### Lookup / insert flow in `CalcResourceValue`

```cpp
int AIQueryService::CalcResourceValue(MapPoint pt, AIResource res,
                                      helpers::OptionalEnum<Direction> direction,
                                      int lastval) const
{
    // Directional incremental form depends on lastval — bypass cache entirely.
    if(direction)
        return ComputeResourceValueUncached(pt, res, direction, lastval);

    const unsigned currentGF = gwb.GetEvMgr().GetCurrentGF();
    const ResourceCacheKey key{pt, res};

    const auto it = resourceValueCache_.find(key);
    if(it != resourceValueCache_.end() && currentGF < it->second.expiresAtGF)
    {
        ++resourceValueCacheHits_;
        return it->second.value;
    }

    MaybeEvictExpired(currentGF);

    const int value       = ComputeResourceValueUncached(pt, res, boost::none, 0xffff);
    const unsigned expiry = currentGF + GetCacheTTL(res, value);
    resourceValueCache_[key] = {value, expiry};
    ++resourceValueCacheMisses_;
    return value;
}
```

`ComputeResourceValueUncached` is the existing computation body moved into a private helper.

### Lazy eviction (`MaybeEvictExpired`)

```cpp
void AIQueryService::MaybeEvictExpired(unsigned currentGF) const
{
    ++resourceValueMissesSinceLastEvict_;

    const bool overHardCap  = resourceValueCache_.size() > kResourceValueCacheHardCap;
    const bool timeForSweep = resourceValueMissesSinceLastEvict_ >= kEvictEveryNMisses;

    if(!overHardCap && !timeForSweep)
        return;

    resourceValueMissesSinceLastEvict_ = 0;

    for(auto it = resourceValueCache_.begin(); it != resourceValueCache_.end();)
    {
        if(currentGF >= it->second.expiresAtGF)
            it = resourceValueCache_.erase(it);
        else
            ++it;
    }

    // Backstop: if still over cap after evicting expired entries, clear entirely.
    if(resourceValueCache_.size() > kResourceValueCacheHardCap)
        resourceValueCache_.clear();
}
```

Constants (at file scope in .cpp):

```cpp
constexpr unsigned kEvictEveryNMisses        = 4096;
constexpr size_t   kResourceValueCacheHardCap = 200'000;
```

The `kPermanentTTL = 1,000,000` entries survive eviction sweeps in almost every case, which is
the desired behavior. They are still evicted if `expiresAtGF` is actually reached (after ~11 h at
25 GF/s) or by the hard-cap backstop.

### Stat accessors (AIQueryService.h)

```cpp
public:
    unsigned long long GetResourceValueCacheHits()   const { return resourceValueCacheHits_; }
    unsigned long long GetResourceValueCacheMisses() const { return resourceValueCacheMisses_; }
    void ResetResourceValueCacheStats() { resourceValueCacheHits_ = resourceValueCacheMisses_ = 0; }
```

Wire the counters into the existing profiler output (same location as other AI perf stats).

## Files Touched

| File | Change |
|------|--------|
| `libs/s25main/ai/AIQueryService.h` | Add cache members, key/hash struct, stat accessors; declare `ComputeResourceValueUncached` and `MaybeEvictExpired` as private. |
| `libs/s25main/ai/AIQueryService.cpp` | Move existing body to `ComputeResourceValueUncached`; implement cache lookup/insert, `GetCacheTTL`, `MaybeEvictExpired`, stats. |
| `libs/s25main/ai/aijh/planning/GlobalPositionFinderStats.h` (or profiler sink) | Add hits/misses to the per-tick report. |

No changes at any call site.

## Directional form — keep as-is

`direction + lastval` calls are the incremental radius-roll optimization used in
`AIResourceMap.cpp:235, 240, 242, 276, 279, 281`. The return value depends on `lastval` which is
not part of the cache key, so these calls are always forwarded to `ComputeResourceValueUncached`
unchanged. Parameters `direction` and `lastval` must remain on the public signature.

## Tests

- All existing tests pass without change (non-directional calls are transparent within TTL;
  directional calls are never cached).
- **Hit counter test**: call `CalcResourceValue(pt, res)` twice in the same frame; assert
  `GetResourceValueCacheHits() == 1` and both return equal values.
- **TTL expiry test**: call once, advance frame by TTL for that resource, call again; assert a
  miss occurs and a fresh value is returned.
- **Permanent zero test**: call with a resource that returns 0 for Gold/Ironore/Coal/Granite/Stones;
  advance frame by `kDefaultTTL + 1`; assert the entry is still a hit (permanent TTL).
- **Eviction test**: insert > `kEvictEveryNMisses` distinct expired points; assert the sweep runs
  and cache size is reduced.

## Validation

- Run `ctest --output-on-failure -j$(nproc)`.
- Re-run AI profiler on the same scenario as `tmp/performance_result.txt`. Expect meaningful
  reduction in `ExecuteGlobalBuildJobs` (currently 20 s) once the cache warms up and hit rate
  dominates. Record pre/post in PR description.

## Risks

- **Staleness**: non-zero deposits for Gold/Ironore/Coal/Granite/Stones expire at 1000 frames
  (~40 s at 25 GF/s), same as before. Zero values effectively never expire — correctness relies
  on the assumption that deposits cannot appear where none existed. This is true for the current
  map model.
- **Permanent TTL overflow**: `expiresAtGF = currentGF + 1,000,000`. On a very long session
  this could overflow `unsigned` (~4 B). At 25 GF/s the game would need to run ~1.9 years
  continuously to overflow. Acceptable.
- **Memory**: hard cap of 200,000 entries × ~20 B ≈ 4 MB per AI. With a backstop clear, growth
  is bounded.
- **Thread safety**: same as before — single-threaded per AI, `mutable` cache does not introduce
  new hazards.

## Follow-ups (out of scope)

- Event-driven invalidation (building placement/destruction, territory change, tree chop, mine
  depletion) for tighter correctness if AI decision quality regresses at long TTLs.
- Per-resource tuning of `kDefaultTTL` (Wood, Plantspace, Borderland) based on profiler deltas.
