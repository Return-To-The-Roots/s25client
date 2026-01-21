# Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# Needs to be configured with @ONLY and called with -DCMAKE_BUILD_TYPE=...

set(CMAKE_HOST_UNIX "@CMAKE_HOST_UNIX@")
set(PROJECT_SOURCE_DIR "@PROJECT_SOURCE_DIR@")
set(RTTR_TRANSLATION_OUTPUT "@RTTR_TRANSLATION_OUTPUT@")
set(RTTR_DATADIR "@RTTR_DATADIR@")
set(RTTR_GAMEDIR "@RTTR_GAMEDIR@")
set(RTTR_OUTPUT_DIR "@RTTR_OUTPUT_DIR@")
set(RTTR_OUTPUT_DIR_RELEASE "@RTTR_OUTPUT_DIR_RELEASE@")
set(RTTR_OUTPUT_DIR_DEBUG "@RTTR_OUTPUT_DIR_DEBUG@")
set(RTTR_OUTPUT_DIR_RELWITHDEBINFO "@RTTR_OUTPUT_DIR_RELWITHDEBINFO@")
set(RTTR_OUTPUT_DIR_MINSIZEREL "@RTTR_OUTPUT_DIR_MINSIZEREL@")

################################################################################
# Copy data files for easier debugging
################################################################################

if(NOT CMAKE_BUILD_TYPE)
    set(CUR_OUTPUT_DIR ${RTTR_OUTPUT_DIR})
    if(NOT CUR_OUTPUT_DIR)
        message(FATAL_ERROR "Output directory not set. There was probably an error")
    endif()
else()
    string(TOUPPER ${CMAKE_BUILD_TYPE} UPPER_BUILD_TYPE)
    set(CUR_OUTPUT_DIR ${RTTR_OUTPUT_DIR_${UPPER_BUILD_TYPE}})
    if(NOT CUR_OUTPUT_DIR)
        message(FATAL_ERROR "Output directory for build type '${CMAKE_BUILD_TYPE}' not set. The build type is probably unsupported")
    endif()
endif()

get_filename_component(RTTR_OUTPUT_PATH "${RTTR_DATADIR}" ABSOLUTE BASE_DIR "${CUR_OUTPUT_DIR}")
file(COPY "${RTTR_TRANSLATION_OUTPUT}/"
    DESTINATION "${RTTR_OUTPUT_PATH}/RTTR/languages"
    FILES_MATCHING PATTERN "*.mo"
)
file(COPY "${PROJECT_SOURCE_DIR}/data/RTTR" DESTINATION "${RTTR_OUTPUT_PATH}")

set(S2_GAME_PATHS ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/S2)
get_filename_component(GAME_OUTPUT_PATH "${RTTR_GAMEDIR}" ABSOLUTE BASE_DIR "${CUR_OUTPUT_DIR}")
if(NOT EXISTS ${GAME_OUTPUT_PATH}/DATA)
    find_path(S2_DATA_DIR DATA/IO.LST PATHS ${S2_GAME_PATHS})
    if(S2_DATA_DIR)
        file(COPY ${S2_DATA_DIR}/DATA DESTINATION ${GAME_OUTPUT_PATH})
    endif()
endif()
if(NOT EXISTS ${GAME_OUTPUT_PATH}/GFX)
    find_path(S2_GFX_DIR GFX/PALETTE/PAL5.BBM PATHS ${S2_GAME_PATHS})
    if(S2_GFX_DIR)
        file(COPY ${S2_GFX_DIR}/GFX DESTINATION ${GAME_OUTPUT_PATH})
    endif()
endif()
