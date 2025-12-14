# Repository Overview

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
- `doc/`, `tools/`, `extras/` – Documentation, developer utilities, release
  scripts, and auxiliary assets.
- `cmake/` & root `CMakeLists.txt` – Build definitions, toolchain helpers,
  and configuration options (optimizations, platform targets, sanitizers).

## Working Guidelines

- Follow the repository coding standards: Allman braces, four-space indent,
  PascalCase for classes/enums, CamelCase for functions, snake_case locals,
  and explicit namespaces (`rttr::`).
- Avoid new global singletons—thread services through constructors or the
  dependency container.
- Respect the asset pipeline: never commit original Settlers II data; place
  your copy next to the `put your S2-Installation in here` marker when
  launching locally.
- Run tests (`ctest`) before submitting changes and call out sanitizer status
  in reviews when relevant.
- Document gameplay/UI impact, toggled CMake options, and follow-up work in
  pull requests; include screenshots or clips for visible changes.

This overview should give coding models enough shared context to navigate
the project, find the right subsystem, and follow existing workflows.

## Documentation Index

- [AddingCustomCampaign](AddingCustomCampaign.md) – Step-by-step guide for
  creating custom campaigns, structuring Lua descriptors, and wiring assets
  into the campaign browser.
- [CodeCoverage](CodeCoverage.md) – Expectations for coverage on feature and
  test code plus instructions for generating `lcov` data locally.
- [combat-passive-active](combat-passive-active.md) – Technical memo on the
  soldier lifecycle from passive healing to active combat resolution.
- [hq_reserves](hq_reserves.md) – Deep dive into how headquarters track,
  refresh, and consume reserve soldiers per rank.
- [military_deployment](military_deployment.md) – Explains garrison regulation
  logic, reinforcement ordering, and related UI controls.
- [nobMilitary](nobMilitary.md) – Comprehensive overview of
  `nobMilitary.cpp`, including frontier tracking, promotions, capture flow,
  and event handling.
- [ResourceLoading](ResourceLoading.md) – Describes archive-based resource
  loading, override order, and naming conventions for assets.
- [Lua main](lua/main.md) – Entry point for the Lua scripting interface,
  covering versioning, lifecycle, and example callbacks.
  - [Lua events](lua/events.md) – Reference for gameplay and settings events
    exposed to scripts.
  - [Lua functions](lua/functions.md) – API surface for `rttr` and player
    objects during setup and runtime.
  - [Lua constants](lua/constants.md) – Enumerations for wares, jobs,
    buildings, and more, mirroring production headers.
