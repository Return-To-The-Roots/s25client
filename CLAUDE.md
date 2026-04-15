# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

**Return To The Roots (RttR)** — an open-source fan rewrite of The Settlers 2 (1996). Requires the original "The Settlers 2 Gold Edition" game data at runtime. C++14, CMake, Boost.

## Build

```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
make -j$(nproc)
```

Key CMake options:
- `-DBUILD_TESTING=ON` (default ON) — build tests
- `-DRTTR_ENABLE_SANITIZERS=ON` — enable UBSAN/ASAN
- `-DRTTR_ENABLE_WERROR=ON` — warnings as errors (used in CI)
- `-DRTTR_ENABLE_COVERAGE=ON` — enable coverage collection

## Tests

```bash
cd build
ctest --output-on-failure -j$(nproc)
# Run a single test binary directly:
./tests/s25Main/testSomeFeature
# Or filter with ctest:
ctest -R <test_name_regex>
```

Test sources mirror the library structure under `tests/`. Framework: Boost.Test.

## Linting & Formatting

```bash
# Format all code (requires clang-format 10):
make clangformat

# Static analysis (requires compile_commands.json):
./tools/runClangTidy.sh

# Full static validation (CI checks):
./tools/ci/staticValidation.sh
```

Config files: `.clang-format`, `.clang-tidy`.

## Architecture

### Directory layout

```
libs/           # Core game libraries
  s25main/      # Main game engine (largest: ~15k LOC)
  common/       # Shared utilities (s25Common, C++17)
  driver/       # Hardware abstraction (audio/video)
extras/         # Tools and platform-specific drivers
  ai-battle/    # Standalone AI battle simulator
  audioDrivers/ # SDL audio driver implementations
  videoDrivers/ # SDL2 / WinAPI video implementations
external/       # Vendored/submodule dependencies
  libutil/      # Logging, cmake helpers
  libsiedler2/  # Settlers 2 asset format I/O
  liblobby/     # Lobby/matchmaking protocol
  proto-repo/   # Protobuf message definitions
  kaguya/       # Lua bindings
tests/          # Mirrors libs/ structure
```

### s25main subsystems

The engine is organized into functional subdirectories under `libs/s25main/`:

| Directory | Purpose |
|-----------|---------|
| `ai/` + `ai/aijh/` | AI system — JH-variant with planning, combat, config (see `AI_COMBAT.md`) |
| `world/` | Game world/map representation and game state |
| `network/` | Multiplayer networking and replay system |
| `pathfinding/` | Movement algorithms |
| `buildings/`, `figures/`, `nodeObjs/` | Game entities |
| `desktops/`, `ingameWindows/`, `controls/` | UI layers |
| `drivers/`, `ogl/` | Rendering abstraction and OpenGL |
| `lua/` | Lua scripting integration |
| `mapGenerator/` | Procedural map generation |
| `notifications/`, `postSystem/` | Observer/messaging infrastructure |
| `addons/` | Game add-ons and balance modifiers |

### Key architectural patterns

- **Drivers as plugins**: audio and video drivers are separate shared libraries loaded at runtime, not linked statically. Implementations live in `extras/audioDrivers/` and `extras/videoDrivers/`.
- **AI module boundary**: The AI JH module (`ai/aijh/`) is isolated — it reads world state through a defined interface and should not call game-mutating functions directly.
- **Protobuf messages**: Network and lobby protocols use `.proto` definitions from the `external/proto-repo` submodule; generated code goes into the build directory.
- **Lua scripting**: Maps can embed Lua scripts; the `lua/` subsystem handles sandboxed execution via kaguya bindings.
- **Replay system**: Deterministic replay is a first-class feature — random seed and all inputs are recorded; the network code and the game loop must remain deterministic.

### AI combat tuning

See `AI_COMBAT.md` for the YAML-based combat configuration format used by the AI battle system in `extras/ai-battle/`.
