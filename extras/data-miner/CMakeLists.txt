# Copyright (C) 2005 - 2024 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(data-miner main.cpp
        DataMiner.cpp
        DataMiner.h
        SnapshotLoader.h
        SnapshotLoader.cpp
)
find_package(yaml-cpp REQUIRED)
find_package(Arrow REQUIRED)

include(FetchContent)
FetchContent_Declare(
        fast-cpp-csv-parser
        GIT_REPOSITORY https://github.com/ben-strasser/fast-cpp-csv-parser.git
        GIT_TAG master
)
FetchContent_MakeAvailable(fast-cpp-csv-parser)

target_link_libraries(
        data-miner
        PRIVATE s25Main
            Boost::program_options
            Boost::nowide
            yaml-cpp
)

target_include_directories(
        data-miner
        PRIVATE ${fast-cpp-csv-parser_SOURCE_DIR}
)
# Add a definition to use C++17 filesystem (required by some CSV parsers)
target_compile_definitions(data-miner PRIVATE USE_CXX17_FILESYSTEM)

if(WIN32)
    include(GatherDll)
    gather_dll_copy(data-miner)
endif()
