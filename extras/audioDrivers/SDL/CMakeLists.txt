set(SDL_BUILDING_LIBRARY ON)
FIND_PACKAGE(SDL_mixer)

IF(NOT SDL_MIXER_FOUND)
	MESSAGE(WARNING ": SDL_mixer library not found: Not building SDL audiodriver")
ELSE()
	IF (WIN32)
        include(GatherDll)
        gather_dll(SDL_MIXER)
        gather_dll_by_name(VORBIS libvorbis-0.dll)
        gather_dll_by_name(VORBISFILE libvorbisfile-3.dll)
        gather_dll_by_name(OGG libogg-0.dll)
	ENDIF ()

	ADD_LIBRARY(audioSDL SHARED ${RTTR_DRIVER_INTERFACE} AudioSDL.cpp AudioSDL.h SoundSDL_Effect.h SoundSDL_Music.h)
	TARGET_LINK_LIBRARIES(audioSDL PRIVATE audiodrv SDL_mixer::SDL_mixer)

	INSTALL(TARGETS audioSDL
		RUNTIME DESTINATION ${RTTR_DRIVERDIR}/audio
		LIBRARY DESTINATION ${RTTR_DRIVERDIR}/audio
	)
ENDIF ()
