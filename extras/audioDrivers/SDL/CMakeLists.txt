# Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

set(SDL_BUILDING_LIBRARY ON)
find_package(SDL_mixer 2.0.1)

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

    add_library(audioSDL SHARED ${RTTR_DRIVER_INTERFACE} AudioSDL.cpp AudioSDL.h)
    target_link_libraries(audioSDL PRIVATE audiodrv SDL_mixer::SDL_mixer)
    enable_warnings(audioSDL)

    install(TARGETS audioSDL
      RUNTIME DESTINATION ${RTTR_DRIVERDIR}/audio
      LIBRARY DESTINATION ${RTTR_DRIVERDIR}/audio
    )
    add_dependencies(drivers audioSDL)
endif()
