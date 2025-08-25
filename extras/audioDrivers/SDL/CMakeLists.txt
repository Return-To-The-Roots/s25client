# Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

set(SDL_BUILDING_LIBRARY ON)
if (CMAKE_SYSTEM_NAME MATCHES "Emscripten")
    set(SDL_MIXER_FOUND TRUE)
else ()
find_package(SDL_mixer 2.0.1)
endif ()
if(NOT SDL_MIXER_FOUND)
    message(WARNING "SDL_mixer library not found: Not building SDL audiodriver")
else()
    if(WIN32)
        include(GatherDll)
        gather_dll(SDL_MIXER)
        gather_dll_by_name(VORBIS libvorbis-0.dll)
        gather_dll_by_name(VORBISFILE libvorbisfile-3.dll)
        gather_dll_by_name(OGG libogg-0.dll)
    endif()

    if (CMAKE_SYSTEM_NAME MATCHES "Emscripten")
        add_library(audioSDL STATIC ${RTTR_DRIVER_INTERFACE} AudioSDL.cpp AudioSDL.h)
        set(USE_FLAGS "--use-port=sdl2_mixer -sSDL2_MIXER_FORMATS=ogg")
        set_target_properties(audioSDL PROPERTIES
                COMPILE_FLAGS "${USE_FLAGS}"
                LINK_FLAGS "${USE_FLAGS}"
        )
        target_link_libraries(audioSDL PRIVATE audiodrv)
    else ()
        add_library(audioSDL SHARED ${RTTR_DRIVER_INTERFACE} AudioSDL.cpp AudioSDL.h)
        target_link_libraries(audioSDL PRIVATE audiodrv SDL_mixer::SDL_mixer)
    endif ()
    enable_warnings(audioSDL)

    install(TARGETS audioSDL
      RUNTIME DESTINATION ${RTTR_DRIVERDIR}/audio
      LIBRARY DESTINATION ${RTTR_DRIVERDIR}/audio
    )
    add_dependencies(drivers audioSDL)
endif()
