# Copyright (C) 2005 - 2024 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

set(SDL2_BUILDING_LIBRARY ON)
if (CMAKE_SYSTEM_NAME MATCHES "Emscripten")
    set(SDL2_FOUND TRUE)
else ()
find_package(SDL2 2.0.5)
endif ()
if(SDL2_FOUND)
    if (CMAKE_SYSTEM_NAME MATCHES "Emscripten")
        add_library(videoSDL2 STATIC ${RTTR_DRIVER_INTERFACE} VideoSDL2.cpp VideoSDL2.h icon.h icon.cpp)
        set(USE_FLAGS "--use-port=sdl2 --use-port=sdl2_image")
        set_target_properties(videoSDL2 PROPERTIES
                COMPILE_FLAGS "${USE_FLAGS}"
                LINK_FLAGS "${USE_FLAGS}"
        )
        target_link_libraries(videoSDL2 PRIVATE videodrv s25util::common Boost::nowide)
    else ()
        add_library(videoSDL2 SHARED ${RTTR_DRIVER_INTERFACE} VideoSDL2.cpp VideoSDL2.h icon.h icon.cpp)
        target_link_libraries(videoSDL2 PRIVATE videodrv s25util::common glad Boost::nowide SDL2::SDL2)
    endif ()
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
