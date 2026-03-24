# Issue 3 Implementation Plan

## Goal

Close issue 3 from `tmp/code_review_230626.md` by finishing the split of
`AIInterface` into clearer role-based boundaries:

- `AIQueryService` for read-only world and player queries
- `AICommandSink` for command emission and chat

The current tree already contains these types, so the remaining work is to make
them the real boundary instead of leaving `AIInterface` as the primary API.

## Current State

The split is partially implemented:

- `libs/s25main/ai/AIQueryService.h`
- `libs/s25main/ai/AICommandSink.h`
- `libs/s25main/ai/AIInterface.h`

`AIInterface` now delegates to `AIQueryService` and inherits `AICommandSink`,
but it still re-exposes nearly the entire old API as a large compatibility
wrapper. That means:

- callers still mostly depend on `AIInterface`
- the read/write split exists in code, but not yet in usage
- the type name `AIInterface` still hides the real ownership model

## Plan

### Phase 1: Inventory the remaining compatibility surface

Status: Done

Files:

- `libs/s25main/ai/AIInterface.h`
- all callers of `AIInterface`

Actions:

- Classify every `AIInterface` method as one of:
  - query-only
  - command-only
  - mixed/unclear
  - compatibility-only
- Identify which call sites already could use `Queries()` or `Commands()`
  without behavior changes.
- Record any methods that still force `AIInterface` to exist as a combined type.

Output:

- method inventory grouped by responsibility
- migration list of low-risk call sites

Result:

- `AIInterface` methods fall into three live groups:
  - query-only wrappers: world/resource/pathfinding/building-state accessors
  - command-only wrappers inherited from `AICommandSink`
  - compatibility-only wrappers on top of `AIQueryService` that still exist
    only because callers have not been migrated
- No mixed read/write helper is currently exposed directly on `AIInterface`;
  the combined type is now mostly a packaging convenience.

Caller inventory:

- Query-heavy callers:
  - `libs/s25main/ai/aijh/runtime/AIResourceMap.cpp`
  - `libs/s25main/ai/aijh/planning/GlobalPositionFinder.cpp`
  - `libs/s25main/ai/aijh/combat/TargetSelector.cpp`
  - `libs/s25main/ai/aijh/combat/TargetSelectorAttrition.cpp`
- Mixed query/command callers:
  - `libs/s25main/ai/aijh/planning/AIConstruction.cpp`
  - `libs/s25main/ai/aijh/planning/Jobs.cpp`
  - `libs/s25main/ai/aijh/combat/AICombatController.cpp`
- Command-only caller:
  - `libs/s25main/lua/LuaInterfaceGame.cpp`

Low-risk migration targets for phase 2:

- convert helper signatures in `GlobalPositionFinder.cpp` and
  `TargetSelector*.cpp` from `AIInterface` to `AIQueryService`
- replace local `AIInterface&` variables in command-heavy code with explicit
  `AIQueryService&` / `AICommandSink&` pairs where responsibilities are already
  separate
- switch `LuaInterfaceGame.cpp` to the command sink for pact responses

### Phase 2: Migrate internal AI call sites to explicit roles

Status: Done

Files:

- `libs/s25main/ai/aijh/**`
- other AI callers under `libs/s25main/ai/**`

Actions:

- Update read-only code paths to use `AIQueryService` explicitly.
- Update mutating code paths to use `AICommandSink` explicitly.
- Prefer narrow helper signatures such as:
  - `const AIQueryService&`
  - `AICommandSink&`
  instead of `AIInterface&`
- Keep migration mechanical; do not combine with behavior changes.

Priority candidates:

- planner/search helpers that only read world state
- combat selection code that only queries
- build/expedition/chat paths that only emit commands

Output:

- fewer direct `AIInterface` dependencies in the AI runtime

Result:

- query-only helper paths now use `AIQueryService` explicitly:
  - `libs/s25main/ai/aijh/runtime/AIResourceMap.{h,cpp}`
  - `libs/s25main/ai/aijh/planning/GlobalPositionFinder.cpp`
  - `libs/s25main/ai/aijh/combat/TargetSelector.cpp`
  - `libs/s25main/ai/aijh/combat/TargetSelectorAttrition.cpp`
- mixed combat code now names read/write roles explicitly via
  `owner_.GetInterface().Queries()` and `owner_.GetInterface().Commands()` in
  `libs/s25main/ai/aijh/combat/AICombatController.cpp`
- the Lua pact-response path now depends on `AICommandSink` rather than the
  combined wrapper in `libs/s25main/lua/LuaInterfaceGame.cpp`
- `AIInterface` is still present, but the migrated call sites now use it
  primarily as an access point to `Queries()` / `Commands()` instead of as the
  main API surface

### Phase 3: Shrink `AIInterface` to a compatibility shell

Status: Done

Files:

- `libs/s25main/ai/AIInterface.h`
- `libs/s25main/ai/AIInterface.cpp`

Actions:

- Remove wrapper methods that no longer have live callers.
- Keep only:
  - construction/setup
  - `Queries()`
  - `Commands()`
  - minimal compatibility methods still needed by unmigrated code
- Group any remaining compatibility methods under a clearly marked section.

Output:

- `AIInterface` reads as a transitional façade instead of a second full API

Result:

- `libs/s25main/ai/AIInterface.h` now keeps only:
  - construction/setup
  - `Queries()`
  - `Commands()`
  - shared `gwb` access
  - the subset of legacy wrapper methods still exercised by unmigrated AI code
- dead wrapper methods with no live callers were removed from the header instead
  of leaving `AIInterface` as a second full query API
- the remaining wrapper section is now explicitly labeled as legacy
  compatibility surface

### Phase 4: Clarify ownership in docs and naming

Status: Done

Files:

- `libs/s25main/ai/S2_AI.md`
- optional local notes near `AIInterface`

Actions:

- Document that:
  - `AIQueryService` owns read-only queries
  - `AICommandSink` owns command emission and chat
  - `AIInterface` is a compatibility wrapper for legacy callers
- If practical, add short section comments in `AIInterface.h` to make that
  transitional role explicit.

Output:

- future readers can understand the split without tracing implementation files

Result:

- `libs/s25main/ai/S2_AI.md` now documents the actual ownership model:
  - `AIQueryService` owns read-only queries
  - `AICommandSink` owns command emission and chat
  - `AIInterface` is a transitional combined facade for legacy callers
- `libs/s25main/ai/AIInterface.h` now carries an explicit class comment telling
  readers to prefer `Queries()` and `Commands()` for new code

### Phase 5: Decide whether `AIInterface` still deserves to exist

Status: Done

Files:

- `libs/s25main/ai/AIInterface.*`

Actions:

- Reassess after migration:
  - if enough callers still need a combined object, keep `AIInterface` as a
    thin façade
  - if not, plan a follow-up removal/more aggressive deprecation pass
- This phase is a decision point, not necessarily a deletion step.

Output:

- explicit decision on the long-term role of `AIInterface`

Decision:

- Keep `AIInterface` for now as a thin facade.
- Reason: there are still multiple direct legacy dependencies in core AI files
  such as:
  - `libs/s25main/ai/aijh/runtime/AIPlayerJH.cpp`
  - `libs/s25main/ai/aijh/planning/AIConstruction.cpp`
  - `libs/s25main/ai/aijh/planning/Jobs.cpp`
  - `libs/s25main/ai/aijh/runtime/AIRoadController.cpp`
  - `libs/s25main/lua/LuaPlayer.cpp`
- Follow-up direction: continue migrating those callers toward
  `AIQueryService` / `AICommandSink`, but do that as a separate cleanup pass.
- After phases 2 and 3, `AIInterface` is already small enough that keeping it
  as a compatibility boundary is lower risk than removing it immediately.

## Done Criteria

Issue 3 should be considered implemented when:

- most AI read-only code depends on `AIQueryService`, not `AIInterface`
- most AI mutating code depends on `AICommandSink`, not `AIInterface`
- `AIInterface` no longer mirrors the entire old surface
- docs describe the real ownership split clearly
- build and integration tests still pass

## Validation

After each phase:

- `cmake --build build --target Test_integration --parallel 2`
- `ctest --test-dir build --output-on-failure -R Test_integration`

## Suggested Commit Order

1. method inventory and no-behavior call-site migrations
2. `AIInterface` API reduction
3. docs and compatibility notes
4. final verification
