# AI Battle Startup Arguments

`extras/ai-battle` runs a headless AI-only game for automated simulations,
statistics collection, replay generation, and save/minimap snapshots. The
executable is defined in `extras/ai-battle/CMakeLists.txt` and its command-line
surface is implemented in `extras/ai-battle/main.cpp`.

Build target:

```sh
cmake --build build --target ai-battle
```

Optional CMake toggle:

```sh
cmake -S . -B build -DRTTR_STRIP_AI_BATTLE=ON
```

When enabled, the build strips debug symbols from the final `ai-battle`
executable after linking. This is useful when `RelWithDebInfo` or other
symbol-heavy builds make the binary much larger on disk.

Typical shape:

```sh
build/extras/ai-battle/ai-battle \
  --map path/to/map.wld \
  --ai aijh aijh \
  --weights_file path/to/weights.yaml \
  --output_path /tmp/ai-battle-run \
  --max_gf 100000 \
  --stats_period 2500 \
  --save_period 25000 \
  --minimap_period 2500 \
  --enabled_event_loggers road,building,combat
```

## Required Runtime Arguments

`--help` and `--version` exit before required-option validation. For normal
runs, provide these arguments.

| Argument | Value | Description |
| --- | --- | --- |
| `--map`, `-m` | path | Map to load. The path is expanded through `RTTRCONFIG.ExpandPath(...)`. |
| `--ai` | one or more names | AI roster. Values are parsed by `ParseAIOptions(...)`; supported names are `aijh`, `dummy`, and `none`. Each parsed AI uses hard difficulty. |
| `--weights_file` | path | Global AI weights YAML file loaded through `applyWeightsCfg(...)`. |
| `--output_path` | directory | Root output directory for run artifacts. The option is declared optional in `main.cpp`, but the code dereferences it unconditionally, so pass it for normal runs. |

`--ai none` creates a locked player slot. `aijh` creates the normal AI player,
and `dummy` creates a dummy AI player.

## Run Control

| Argument | Default | Description |
| --- | --- | --- |
| `--max_gf` | max unsigned integer | Maximum game frame to run before stopping. The game can stop earlier when the objective is completed. |
| `--random_init` | high-resolution clock value | Seed for the global random number generator. The executable prints the chosen value at startup. Use a fixed value for reproducibility. |
| `--start_from_save` | unset | Savegame path to load before running. When present, the savegame contents are loaded and CLI AI settings are reapplied to matching player slots where possible. |
| `--replay` | unset | Replay output path. When set, `HeadlessGame::RecordReplay(...)` records replay data using the selected random seed. |

## Game Setup

| Argument | Default | Valid values | Description |
| --- | --- | --- | --- |
| `--objective` | `none` | `none`, `domination`, `conquer` | Sets `GlobalGameSettings::objective` to none, total domination, or conquer three quarters. Unknown values terminate the run with an error. |
| `--start_wares` | `alot` | `vlow`, `low`, `normal`, `alot` | Sets starting wares. Unknown values terminate the run with an error. |
| `--player_weights` | unset | `<player>=<file>` entries | Applies per-player AI weight overrides after the global weights file. Player indices are 1-based and must be within `MAX_PLAYERS`. Multiple entries are accepted. |

`main.cpp` also forces these settings for headless AI battle runs:

| Setting | Forced value |
| --- | --- |
| Exploration | disabled |
| `AddonId::INEXHAUSTIBLE_MINES` | selection `1` |
| `AddonId::DEMOLITION_PROHIBITION` | selection `2` |
| `AddonId::MILITARY_HITPOINTS` | selection `2` |

## Statistics And Snapshots

All period arguments are expressed in game frames. If omitted, the CLI stores
`0` in the corresponding `STATS_CONFIG` field.

| Argument | Default in `main.cpp` | Effect |
| --- | --- | --- |
| `--stats_period` | `0` when omitted | Controls AI stats reporting cadence. If it is `0`, AI stats are saved once at the end of the run. |
| `--debug_stats_period` | `0` when omitted | Controls AI debug stats cadence. |
| `--save_period` | `0` when omitted | When positive, writes savegames at game frame `1` and every multiple of the period. |
| `--minimap_period` | `0` when omitted | When positive, writes minimap bitmap snapshots at matching game frames. |

Output subdirectories are created under `--output_path`:

| Path | Contents |
| --- | --- |
| `<output_path>/stats/` | AI stats, event logs, and `players.json`. |
| `<output_path>/saves/` | Periodic saves named `ai_run_<gameframe>.sav`. |
| `<output_path>/minimaps/` | Periodic minimap bitmaps named `ai_map_<gameframe>.bmp`. |
| `<output_path>/logs/` | Runtime log files via `LOG.setLogFilepath(...)`. |

The run loop also stops early when only one active player remains and the
current frame is at a stats boundary. With `--stats_period 0`, that winner
condition can be checked every frame.

## Event Logger Controls

Event loggers write into `<output_path>/stats/` unless disabled.

| Argument | Description |
| --- | --- |
| `--disable_event_logging` | Disables all event logger output. |
| `--enabled_event_loggers` | Restricts output to the specified logger names. Accepts whitespace-separated entries and comma-separated entries. |

`--disable_event_logging` and `--enabled_event_loggers` are mutually exclusive;
using both is a parse-time error.

Supported logger names come from `GetSupportedEventLoggerNames()` and currently
include:

```text
building, combat, country, country-plot, military, road, tool-priority, troops-limit, ware
```

The parser also accepts normalized forms such as underscores instead of
hyphens, optional whitespace removal, plural aliases where implemented, and an
optional `eventlogger` suffix.

## Informational Options

| Argument | Description |
| --- | --- |
| `--help`, `-h` | Prints the generated Boost program_options help and exits. |
| `--version` | Prints RTTR title, version, revision, compiler, and OS, then exits. |

If the executable is started with no arguments, it prints the option list to
stderr and exits with status `1`.
