
FIND_PACKAGE(SDL)
FIND_PACKAGE(SDL_mixer)

IF (SDL_FOUND)
    CORRECT_LIB(SDL_LIBRARY SDL)

    IF (SDLMIXER_FOUND)
        CORRECT_LIB(SDLMIXER_LIBRARY SDL_mixer)

        INCLUDE_DIRECTORIES("${PROJECT_SOURCE_DIR}/driver/src"
                            "${PROJECT_SOURCE_DIR}/libendian/src"
							${SDL_INCLUDE_DIR}
							${SDLMIXER_INCLUDE_DIR}
                           )
        
        INCLUDE(AddFileDependencies)
        ADD_FILE_DEPENDENCIES("${PROJECT_SOURCE_DIR}/build_version.cpp" "${CMAKE_BINARY_DIR}/build_version.h")
        
        ADD_LIBRARY(audioSDL SHARED SDL.cpp SDL.h main.h SoundSDL_Music.cpp SoundSDL_Music.h SoundSDL_Effect.cpp SoundSDL_Effect.h ${PROJECT_SOURCE_DIR}/build_version.cpp ../../../src/DriverInterface.cpp ../../../src/tempname.cpp ../../../src/tempname.h)
        ADD_DEPENDENCIES(audioSDL updateversion)
        IF ( NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows" )
            CHECK_CXX_COMPILER_FLAG("-fPIC" FPIC_FLAGS)
            IF (FPIC_FLAGS)
                ADD_DEFINITIONS(-fPIC)
            ENDIF (FPIC_FLAGS)
        ENDIF ( NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows" )

        TARGET_LINK_LIBRARIES(audioSDL audiodrv endian ${SDL_LIBRARY} ${SDLMIXER_LIBRARY})
        
		SET(SL_PREFIX ${CMAKE_SHARED_LIBRARY_PREFIX})
        IF ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
            ADD_DEFINITIONS(-DBUILD_DLL)
            SET(SL_PREFIX "")
        ENDIF ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
            
		IF (MSVC)
			SET_TARGET_PROPERTIES(audioSDL PROPERTIES
				RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}/../.."
				RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}/../.."
			)
		ELSE()
			ADD_CUSTOM_COMMAND(TARGET audioSDL POST_BUILD COMMAND ln -fvs SDL/src/${SL_PREFIX}audioSDL${CMAKE_SHARED_LIBRARY_SUFFIX} ../../audioSDL${CMAKE_SHARED_LIBRARY_SUFFIX})
		ENDIF()

        INSTALL(TARGETS audioSDL 
            RUNTIME DESTINATION ${LIBDIR}/driver/audio 
            LIBRARY DESTINATION ${LIBDIR}/driver/audio
            ARCHIVE DESTINATION ${LIBDIR}/driver/audio
        )
    ELSE (SDLMIXER_FOUND)
        MESSAGE(WARNING ": SDL_mixer library not found: Not building SDL audiodriver")
    ENDIF (SDLMIXER_FOUND)
ELSE (SDL_FOUND)
    MESSAGE(WARNING ": SDL library not found: Not building SDL audiodriver")
ENDIF (SDL_FOUND)

#################################################################################
