# Copyright (C) 2005 - 2024 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

set(SDL2_BUILDING_LIBRARY ON)
find_package(SDL2 2.0.5)

if(SDL2_FOUND)
    add_library(videoSDL2 SHARED ${RTTR_DRIVER_INTERFACE} VideoSDL2.cpp VideoSDL2.h icon.h icon.cpp)

    if(RTTR_OPENGL STREQUAL "GL4ES")
        if(NOT TARGET GL)
            message(STATUS "Fetching gl4es from repository...")
            include(FetchContent)
            FetchContent_Declare(
              gl4es
              GIT_REPOSITORY https://github.com/ptitSeb/gl4es
              GIT_TAG master
            )
            FetchContent_MakeAvailable(gl4es)
            target_include_directories(videoSDL2 PRIVATE
              ${gl4es_SOURCE_DIR}/include
            )
        endif()
        # gl4es generates "GL" library
        target_link_libraries(videoSDL2 PRIVATE videodrv s25util::common glad Boost::nowide SDL2::SDL2 GL)
    else()
        target_link_libraries(videoSDL2 PRIVATE videodrv s25util::common glad Boost::nowide SDL2::SDL2)
    endif()
    enable_warnings(videoSDL2)

    if(WIN32)
        include(GatherDll)
        gather_dll_by_name(SDL2 SDL2.dll)
    endif()

    install(TARGETS videoSDL2
      RUNTIME DESTINATION ${RTTR_DRIVERDIR}/video
      LIBRARY DESTINATION ${RTTR_DRIVERDIR}/video
    )
    add_dependencies(drivers videoSDL2)
endif()
