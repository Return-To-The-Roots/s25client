#################################################################################
### $Id: CMakeLists.txt 6941 2010-12-24 07:49:50Z FloSoft $
#################################################################################

INCLUDE(FindSDL)
INCLUDE(FindSDL_mixer)

IF (SDL_FOUND)
    CORRECT_LIB(SDL_LIBRARY SDL)

    IF (SDLMIXER_FOUND)
        CORRECT_LIB(SDLMIXER_LIBRARY SDL_mixer)

        INCLUDE_DIRECTORIES("../../../src"
                            "${PROJECT_SOURCE_DIR}/libendian/src"
                           )
        
        INCLUDE(AddFileDependencies)
        ADD_FILE_DEPENDENCIES("${PROJECT_SOURCE_DIR}/build_version.cpp" "${CMAKE_BINARY_DIR}/build_version.h")
        
        ADD_LIBRARY(audioSDL SHARED SDL.cpp SDL.h main.h SoundSDL_Music.cpp SoundSDL_Music.h SoundSDL_Effect.cpp SoundSDL_Effect.h ${PROJECT_SOURCE_DIR}/build_version.cpp ../../../src/DriverInterface.cpp ../../../src/tempname.cpp ../../../src/tempname.h)
    
        ADD_FLAGS(CMAKE_C_FLAGS -I${SDL_INCLUDE_DIR} -I${SDLMIXER_INCLUDE_DIR})
        ADD_FLAGS(CMAKE_CXX_FLAGS -I${SDL_INCLUDE_DIR} -I${SDLMIXER_INCLUDE_DIR})

        TARGET_LINK_LIBRARIES(audioSDL audiodrv endian ${SDL_LIBRARY} ${SDLMIXER_LIBRARY})
        
        IF ( "${COMPILEFOR}" STREQUAL "windows")
            ADD_DEFINITIONS(-DBUILD_DLL)
            SET(CMAKE_SHARED_LIBRARY_PREFIX "")
        ENDIF ( "${COMPILEFOR}" STREQUAL "windows")
            
        ADD_CUSTOM_COMMAND(TARGET audioSDL POST_BUILD COMMAND ln -fvs SDL/src/${CMAKE_SHARED_LIBRARY_PREFIX}audioSDL${CMAKE_SHARED_LIBRARY_SUFFIX} ../../audioSDL${CMAKE_SHARED_LIBRARY_SUFFIX})

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
