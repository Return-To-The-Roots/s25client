# Copyright (C) 2018 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

option(RTTR_USE_SYSTEM_BENCHMARK "Use system installed google benchmark. Fails if not found!" "${RTTR_USE_SYSTEM_LIBS}")
if(RTTR_USE_SYSTEM_BENCHMARK)
  find_package(benchmark REQUIRED)
else()
    include(FetchContent)
    FetchContent_Declare(
        GoogleBenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.5.2
    )
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "")
    set(BENCHMARK_ENABLE_INSTALL  OFF CACHE BOOL "")
    FetchContent_MakeAvailable(GoogleBenchmark)
endif()

file(GLOB sources *.cpp)
set(benchmarksCommands "")
foreach(src IN LISTS sources)
  get_filename_component(name ${src} NAME_WE)
  set(name BM_${name})
  add_executable(${name} ${src})
  target_link_libraries(${name} PRIVATE s25Main testHelpers testConfig benchmark::benchmark benchmark::benchmark_main)
  list(APPEND benchmarksCommands COMMAND ${name})
endforeach()

add_custom_target(RUN_BENCHMARKS ${benchmarksCommands})
