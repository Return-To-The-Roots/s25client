set(SDL_BUILDING_LIBRARY ON)
find_package(SDL_mixer 1)

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

  add_library(audioSDL SHARED ${RTTR_DRIVER_INTERFACE} AudioSDL.cpp AudioSDL.h SoundSDL_Effect.h SoundSDL_Music.h)
  target_link_libraries(audioSDL PRIVATE audiodrv SDL1_mixer::SDL_mixer)

  install(TARGETS audioSDL
    RUNTIME DESTINATION ${RTTR_DRIVERDIR}/audio
    LIBRARY DESTINATION ${RTTR_DRIVERDIR}/audio
  )
  add_dependencies(drivers audioSDL)
endif()

set(SDL2_BUILDING_LIBRARY ON)
find_package(SDL_mixer 2)

if(NOT SDL_MIXER_FOUND)
  message(WARNING "SDL2_mixer library not found: Not building SDL audiodriver")
else()
  if(WIN32)
    include(GatherDll)
    gather_dll(SDL2_MIXER)
    gather_dll_by_name(VORBIS libvorbis-0.dll)
    gather_dll_by_name(VORBISFILE libvorbisfile-3.dll)
    gather_dll_by_name(OGG libogg-0.dll)
  endif()

  add_library(audioSDL2 SHARED ${RTTR_DRIVER_INTERFACE} AudioSDL.cpp AudioSDL.h SoundSDL_Effect.h SoundSDL_Music.h)
  target_link_libraries(audioSDL2 PRIVATE audiodrv SDL2_mixer::SDL_mixer)

  install(TARGETS audioSDL2
    RUNTIME DESTINATION ${RTTR_DRIVERDIR}/audio
    LIBRARY DESTINATION ${RTTR_DRIVERDIR}/audio
  )
  add_dependencies(drivers audioSDL2)
endif()
