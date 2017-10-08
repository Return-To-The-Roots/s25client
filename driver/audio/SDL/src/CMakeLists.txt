FIND_PACKAGE(SDL)
FIND_PACKAGE(SDL_mixer)

IF(NOT SDL_FOUND)
	MESSAGE(WARNING ": SDL library not found: Not building SDL audiodriver")
ELSEIF(NOT SDLMIXER_FOUND)
	MESSAGE(WARNING ": SDL_mixer library not found: Not building SDL audiodriver")
ELSE()
	FIND_PACKAGE(Boost 1.55.0 REQUIRED COMPONENTS filesystem system)
	CORRECT_LIB(SDL_LIBRARY SDL)
	CORRECT_LIB(SDLMIXER_LIBRARY SDL_mixer)

	INCLUDE_DIRECTORIES(SYSTEM
						${SDL_INCLUDE_DIR}
						${SDLMIXER_INCLUDE_DIR}
						${Boost_INCLUDE_DIR}
					   )
	
	IF (WIN32)
		ADD_DEFINITIONS(-DBUILD_DLL)
        include(GatherDll)
        gather_dll(SDLMIXER)
        gather_dll_by_name(VORBIS libvorbis-0.dll)
        gather_dll_by_name(VORBISFILE libvorbisfile-3.dll)
        gather_dll_by_name(OGG libogg-0.dll)
	ENDIF ()
		
    FILE(GLOB LIB_SRC *.cpp *.h)
	ADD_LIBRARY(audioSDL SHARED ${LIB_SRC} ${PROJECT_SOURCE_DIR}/driver/src/DriverInterface.cpp ${COMMON_SRC})
	TARGET_LINK_LIBRARIES(audioSDL audiodrv s25util ${SDL_LIBRARY} ${SDLMIXER_LIBRARY} ${Boost_LIBRARIES})

	INSTALL(TARGETS audioSDL DESTINATION ${RTTR_DRIVERDIR}/audio)
ENDIF ()
