# AI JH Module Map

This directory contains the JH production AI split by domain rather than by
historical file growth.

## Directory Layout

- `runtime/`
  - coordinator and live AI state
  - examples: `AIPlayerJH`, `AIAdjuster`, `AIResourceMap`,
    `AIRoadController`, `AIMap`
- `planning/`
  - build planning, position search, and job execution
  - examples: `AIConstruction`, `BuildingPlanner`, `BuildingCalculator`,
    `GlobalPositionFinder`, `Jobs`, `PositionSearch`
- `combat/`
  - combat orchestration and target-selection strategies
  - examples: `AICombatController`, `TargetSelector*`
- `config/`
  - config models, parsing, and weight/default handling
  - examples: `AIConfig`, `WeightParams`, `WeightParser`
- `debug/`
  - telemetry, stats output, and debug-specific configuration
  - examples: `AIStatsReporter`, `StatsConfig`

## Dependency Direction

The intended dependency direction is:

- `runtime` may depend on `planning`, `combat`, `config`, and `debug`
- `planning` may depend on `config` and selected runtime coordination types
- `combat` may depend on `config` and selected runtime coordination types
- `debug` may depend on runtime state for reporting, but should not drive
  planning or combat policy
- `config` should stay low-level and reusable

The split is primarily about readability and ownership. Cross-folder includes
are still allowed when needed, but new code should avoid turning the
subdirectories back into a flat dependency mesh.

## Placement Rules

When adding a new file:

- put it in `runtime/` if it owns frame-to-frame AI state or coordinates other
  modules
- put it in `planning/` if it decides where/how to build or manages queued job
  execution
- put it in `combat/` if it selects, evaluates, or executes attack strategy
- put it in `config/` if it models or parses AI tuning data
- put it in `debug/` if it exists only for telemetry, diagnostics, or
  developer-facing output

If a file appears to belong to multiple domains, prefer:

1. the folder matching its primary responsibility
2. a narrow helper split if two responsibilities are equally strong
3. documenting the exception explicitly rather than silently broadening a
   folder's meaning
