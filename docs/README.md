# Documentation

This directory is the canonical home for repository documentation. The files
are grouped by topic so architecture notes, gameplay rules, AI internals, Lua
references, and contributor-facing guides are easier to browse.

## Repository Overview

Return To The Roots (RTTR) recreates The Settlers II with a modernized
codebase, networking layer, and tooling. This repository holds the core
`s25client` game alongside launcher glue, UI/gameplay systems, asset
loaders, and developer documentation. Original Gold Edition data is still
required, but RTTR provides upgrades such as internet multiplayer, wide
platform support, and optional sanitizers for development builds.

## Build & Test Quickstart

- Configure once per build type: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug`.
- Rebuild or iterate with `cmake --build build --target s25client` (pass
  `--config Release` on multi-config generators).
- Launch via `build/libs/s25client/s25client` or wrap packaged settings with
  `start.sh`.
- Run the Boost.Test suite using `ctest --test-dir build --output-on-failure`.
- Enable sanitizers locally with `-DRTTR_ENABLE_SANITIZERS=ON` when tracking
  undefined behavior or memory bugs.

## Repository Layout

- `libs/s25client` – SDL-based launcher and platform glue that boots the
  engine, sets up networking, and interacts with OS services.
- `libs/s25main` – UI layer, gameplay loop, AI, and the majority of game
  logic (settlements, combat, economy, replay handling, etc.).
- `libs/common` – Shared utilities, math helpers, serialization, and other
  cross-module foundations.
- `libs/rttrConfig` – Configuration helpers, profile handling, and asset
  pipeline utilities used by launchers and tooling.
- `libs/libGamedata` – Data loading abstractions for legacy Settlers II
  assets, providing translation layers for graphics, maps, and UI strings.
- `libs/driver` – Platform drivers (audio, input, rendering surfaces,
  file/dialog helpers) shared between the launcher and main game.
- `external/` – Vendored third-party dependencies pulled as git submodules.
- `data/` – Packaged assets such as localization, graphics, and maps; adjust
  these only via the approved asset toolchain.
- `tests/` – Boost.Test suites mirroring the library layout (e.g.
  `tests/s25Main/...`) and using fixtures derived from `rttr::test::Fixture`.
- `docs/`, `tools/`, `extras/` – Documentation, developer utilities, release
  scripts, and auxiliary assets.
- `cmake/` and the root `CMakeLists.txt` – Build definitions, toolchain
  helpers, and configuration options (optimizations, platform targets,
  sanitizers).

## Working Guidelines

- Follow the repository coding standards: Allman braces, four-space indent,
  PascalCase for classes and enums, CamelCase for functions, snake_case locals,
  and explicit namespaces (`rttr::`).
- Avoid new global singletons. Thread services through constructors or the
  dependency container.
- Respect the asset pipeline: never commit original Settlers II data; place
  your copy next to the `put your S2-Installation in here` marker when
  launching locally.
- Run tests (`ctest`) before submitting changes and call out sanitizer status
  in reviews when relevant.
- Document gameplay or UI impact, toggled CMake options, and follow-up work in
  pull requests. Include screenshots or clips for visible changes.

## Topic Map

### Architecture

- [Resource loading](architecture/resource-loading.md) – Archive loading,
  override order, and asset naming conventions.

### Campaigns And Lua

- [Custom campaigns](campaigns/custom-campaigns.md) – Creating and wiring a
  campaign into the browser.
- [Lua scripting](lua/README.md) – Entry point for the Lua interface.
- [Lua events](lua/events.md) – Gameplay and settings events exposed to
  scripts.
- [Lua functions](lua/functions.md) – API surface for `rttr` and player
  objects.
- [Lua constants](lua/constants.md) – Script-visible wares, jobs, buildings,
  and other enums.

### Gameplay

- [Combat and healing flow](gameplay/combat-healing-flow.md) – Passive healing
  and active combat transitions.
- [Headquarters reserves](gameplay/headquarters-reserves.md) – How reserve
  soldiers are tracked and consumed.
- [Construction economy](gameplay/economy/construction-economy.md) – Materials
  and builder flow for new structures.
- [Roads](gameplay/economy/roads.md) – Road network mechanics, transport
  segments, and AI road construction flow.
- [Food economy](gameplay/economy/food-economy.md) – Food production and
  consumption chain summary.
- [Mining economy](gameplay/economy/mining-economy.md) – Ore and coal supply
  overview.
- [Tools and goods](gameplay/economy/tools-and-goods.md) – Production chains
  for tools and manufactured wares.
- [Military deployment](gameplay/military/deployment.md) – Regulation and
  ordering of garrison troops.
- [Understaffed military assignment](gameplay/military/understaffed-assignment.md)
  – Soldier allocation when all garrisons cannot be fully staffed.
- [Military building logic](gameplay/military/nob-military.md) – Detailed
  walkthrough of `nobMilitary.cpp`.
- [Enemy building reachability](gameplay/military/enemy-building-reachability.md)
  – Frontier classification and the optional path-reachability check.

### AI

- [AI adjustments](ai/adjustments.md) – Adjustment rules and tool-priority
  behavior.
- [AI gold distribution](ai/gold-distribution.md) – How military coin supply
  is enabled, requested, and routed between eligible buildings.
- [AI configuration](ai/configuration.md) – Configuration surface and loaded
  settings.
- [Attack target selection](ai/attack-target-selection.md) – Target-choice
  pipeline and heuristics.
- [Construction mechanics](ai/construction-mechanics.md) – Queueing and
  deduplication of AI construction work.
- [Road route selection](ai/road-route-selection.md) – How the AI scores
  nearby flag connections and picks new road segments.
- [Position finding](ai/position-finding.md) – Finder logic for strategic
  placement.
- [Statistics logging](ai/statistics-logging.md) – AIPlayerJH stats collection
  and outputs.

### Development And Tools

- [Code coverage](development/code-coverage.md) – Coverage expectations and
  local generation workflow.
- [Event loggers](development/event-loggers.md) – Logging infrastructure for
  gameplay and AI analysis.
- [Map file parsing and minimap rendering](development/map-file-parsing-and-minimap-rendering.md)
  – How `.WLD` / `.SWD` files are parsed, which layers RTTR imports into the
  live world, and how preview and ingame minimaps are rendered.
- [Road and flag rendering](development/road-and-flag-rendering.md) – How
  terrain roads, flag sprites, fog-of-war state, and road previews are drawn.
- [Building and site rendering](development/building-and-construction-site-rendering.md)
  – How finished building sprites, construction progress, and fog-of-war
  building memories are drawn.
- [AI battle startup arguments](tools/ai-battle.md) – Headless AI simulation
  CLI options, outputs, stats periods, and event logger controls.
- [Data extractor](tools/data-extractor.md) – Snapshot generation and exported
  data formats.
