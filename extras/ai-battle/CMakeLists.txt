# Copyright (C) 2005 - 2024 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

option(RTTR_STRIP_AI_BATTLE "Strip debug symbols from the ai-battle executable after linking" OFF)

add_executable(ai-battle main.cpp HeadlessGame.cpp)
find_package(yaml-cpp REQUIRED)
target_link_libraries(ai-battle PRIVATE s25Main Boost::program_options Boost::nowide yaml-cpp)
target_include_directories(ai-battle PRIVATE
    ${PROJECT_SOURCE_DIR}/libs/s25main
    ${PROJECT_SOURCE_DIR}/libs/rttrConfig/src
)

if(WIN32)
    include(GatherDll)
    gather_dll_copy(ai-battle)
endif()

if(RTTR_STRIP_AI_BATTLE)
    if(CMAKE_STRIP)
        add_custom_command(
            TARGET ai-battle POST_BUILD
            COMMAND ${CMAKE_STRIP} $<TARGET_FILE:ai-battle>
            COMMENT "Stripping ai-battle executable"
        )
    else()
        message(WARNING "RTTR_STRIP_AI_BATTLE is ON, but CMAKE_STRIP is not available")
    endif()
endif()
