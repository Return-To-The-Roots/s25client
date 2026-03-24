# Issue 2 Implementation Plan

## Goal

Close issue 2 from `tmp/code_review_230626.md` by making the `aijh/`
subdirectory structure the canonical, fully integrated layout for the JH AI
module.

The target structure is already present:

- `libs/s25main/ai/aijh/runtime/`
- `libs/s25main/ai/aijh/planning/`
- `libs/s25main/ai/aijh/combat/`
- `libs/s25main/ai/aijh/config/`
- `libs/s25main/ai/aijh/debug/`

So the remaining work is not the initial split, but finishing the integration
so the new layout is obvious, stable, and low-friction for future changes.

## Current State

The code is already grouped broadly by role:

- `runtime/`: `AIPlayerJH`, `AIAdjuster`, `AIResourceMap`, `AIRoadController`
- `planning/`: `AIConstruction`, `BuildingPlanner`, `BuildingCalculator`,
  `GlobalPositionFinder`, `PositionSearch`, `Jobs`, `PlannerHelper`
- `combat/`: `AICombatController`, `TargetSelector*`
- `config/`: `AIConfig`, `WeightParams`, `WeightParser`
- `debug/`: `AIStatsReporter`, `StatsConfig`

This means issue 2 is partially addressed structurally, but it still needs a
"definition of done" pass.

## Plan

### Phase 1: Validate ownership and folder boundaries

Status: Done

Files:

- `libs/s25main/ai/aijh/**`

Actions:

- Review each file against its current folder and confirm that its primary
  responsibility matches the directory name.
- Call out exceptions explicitly, especially:
  - `runtime/AIMap.h`
  - `runtime/AIPlayerJHStats.cpp`
  - any helper used by multiple domains but stored in one domain folder
- Decide whether each exception should:
  - stay where it is with a short rationale
  - move to another folder
  - be split into a narrower helper

Output:

- a file-to-domain ownership table
- a short list of exceptions that remain intentional

Result:

| Domain | Files | Notes |
| --- | --- | --- |
| `runtime` | `AIPlayerJH.*`, `AIAdjuster.cpp`, `AIResourceMap.*`, `AIRoadController.*`, `AIMap.h`, `AIPlayerJHStats.cpp` | runtime orchestration, mutable AI state, and compatibility methods that still belong to the coordinator surface |
| `planning` | `AIConstruction.*`, `BuildingCalculator.*`, `BuildingPlanner.*`, `GlobalPositionFinder.*`, `Jobs.*`, `PlannerHelper.*`, `PositionSearch.*` | building placement, connection jobs, and search/planning helpers |
| `combat` | `AICombatController.*`, `TargetSelector*.cpp` | combat-mode control and target-selection strategies |
| `config` | `AIConfig.*`, `WeightParams.*`, `WeightParser.*` | config storage, defaults, and parsing |
| `debug` | `AIStatsReporter.*`, `StatsConfig.*` | telemetry output and stats configuration |

Intentional exceptions:

- `runtime/AIPlayerJHStats.cpp` stays in `runtime/` for now because it only
  contains compatibility methods on `AIPlayerJH` that forward into
  `AIStatsReporter`. The heavy telemetry logic already lives in `debug/`.
- `runtime/AIMap.h` stays in `runtime/` because it is shared mutable state used
  directly by the runtime coordinator and adjuster path, not a standalone
  planning or config module.
- `runtime/AIResourceMap.*` stays in `runtime/` because it is part of the live
  world-state cache owned by the running AI, even though it supports planning
  decisions.

### Phase 2: Remove stale flat-layout assumptions

Status: Done

Files:

- `libs/s25main/CMakeLists.txt`
- all includes referencing `ai/aijh/...`
- docs mentioning the old flat tree

Actions:

- Search for old include paths, comments, or build references that still assume
  `aijh/` is flat.
- Normalize includes so they reflect the domain structure consistently.
- Ensure build lists are grouped by domain in CMake so the structure is visible
  in the build definition too.

Output:

- no old flat-path references
- domain-grouped source lists in CMake

Result:

- Removed the empty `AddDirectory(ai/aijh)` call from
  `libs/s25main/CMakeLists.txt` so the top-level build no longer suggests a
  flat source bucket.
- Updated stale flat-layout references in:
  - `doc/ai_config.md`
  - `doc/stats.md`
  - `doc/attack_target.md`
  - `doc/pos_finder.md`
  - `doc/combat-passive-active.md`
  - `doc/nobMilitary.md`
  - `libs/s25main/ai/S2_AI.md`
- Verified that no `doc/`, `libs/s25main/`, `extras/`, or `tests/` files still
  reference `libs/s25main/ai/aijh/<flat-file>` paths.

### Phase 3: Make the structure discoverable for readers

Status: Done

Files:

- `libs/s25main/ai/S2_AI.md`
- optionally a small local README in `libs/s25main/ai/aijh/`

Actions:

- Add a short overview of what belongs in each subdirectory.
- Document the intended dependency direction:
  - `runtime` may depend on `planning`, `combat`, `config`, `debug`
  - `planning` should not depend on `debug`
  - `config` should stay low-level and reusable
- Document where new files should go.

Output:

- one short module map for future contributors

Result:

- Added `libs/s25main/ai/aijh/README.md` with:
  - folder-to-responsibility mapping
  - intended dependency direction
  - placement rules for new files
- Updated `libs/s25main/ai/S2_AI.md` to point readers at the local module map.

### Phase 4: Tighten boundaries where the structure is still cosmetic

Status: Done

Files:

- cross-domain headers under `libs/s25main/ai/aijh/**`

Actions:

- Review cross-folder includes and identify places where the new directory split
  is only cosmetic because dependencies still point everywhere.
- Prefer forward declarations and narrower headers where practical.
- Keep this phase mechanical; do not mix in behavior changes.

Output:

- fewer unnecessary cross-domain includes
- clearer dependency direction between folders

Result:

- `planning/BuildingCalculator.h` no longer includes
  `runtime/AIPlayerJH.h` or `config/AIConfig.h` just to expose declarations.
  It now includes only the concrete types it needs and forward-declares
  `AIPlayerJH` and `Inventory`.
- `planning/PlannerHelper.h` no longer includes
  `runtime/AIPlayerJH.h`; it now forward-declares `AIPlayerJH` and includes
  only `BuildingType.h`.
- `planning/PlannerHelper.cpp` now pulls in `AIPlayerJH.h` privately where the
  implementation actually needs the full runtime type.

### Phase 5: Verify with build and focused tests

Status: Done

Validation:

- `cmake --build build --parallel 2`
- `ctest --test-dir build --output-on-failure -R Test_integration`

Actions:

- Rebuild after cleanup.
- Run the integration target once the layout cleanup is complete.

Output:

- confirmation that the structural cleanup did not regress behavior

Result:

- `cmake --build build --parallel 2` succeeded.
- `ctest --test-dir build --output-on-failure -R Test_integration` succeeded.
- The `aijh` structure cleanup is now reflected in ownership notes, docs, and
  boundary cleanup without regressing the integration target.

## Done Criteria

Issue 2 should be considered fully implemented when all of the following are
true:

- every `aijh` file has an unambiguous home in one of the five subdirectories
- there are no stale references to the old flat layout
- the build files and docs reflect the split clearly
- cross-domain includes are reasonable rather than arbitrary
- the project builds and `Test_integration` passes

## Suggested Commit Order

1. ownership review and small file moves only
2. include-path and CMake cleanup
3. docs/update of module map
4. boundary-tightening cleanup
5. validation and final polish
