# Copyright (C) 2005 - 2024 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

add_executable(data-miner main.cpp
        DataMiner.cpp
        DataMiner.h
        ParquetDataMiner.cpp
        ParquetDataMiner.h)
find_package(yaml-cpp REQUIRED)
find_package(Arrow REQUIRED)
find_package(Parquet REQUIRED)

target_link_libraries(
        data-miner
        PRIVATE s25Main
            Boost::program_options
            Boost::nowide
            yaml-cpp
            Arrow::arrow_shared
            Parquet::parquet_shared
)

target_include_directories(
        data-miner
        PRIVATE
            ${ARROW_INCLUDE_DIR}
            ${PARQUET_INCLUDE_DIR}
)

if(WIN32)
    include(GatherDll)
    gather_dll_copy(data-miner)
endif()
