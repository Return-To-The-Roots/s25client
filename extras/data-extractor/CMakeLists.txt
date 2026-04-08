# Copyright (C) 2005 - 2024 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(data-extractor main.cpp
        SnapshotLoader.h
        SnapshotLoader.cpp
)
find_package(yaml-cpp REQUIRED)
#find_package(Arrow REQUIRED)
find_package(nlohmann_json REQUIRED)

target_link_libraries(
        data-extractor
        PRIVATE s25Main
            Boost::program_options
            Boost::nowide
            yaml-cpp
            nlohmann_json::nlohmann_json
)

target_include_directories(
        data-extractor
        PRIVATE ${CMAKE_SOURCE_DIR}/libs/s25main
)
# Add a definition to use C++17 filesystem (required by some CSV parsers)
target_compile_definitions(data-extractor PRIVATE USE_CXX17_FILESYSTEM)

if(WIN32)
    include(GatherDll)
    gather_dll_copy(data-extractor)
endif()
