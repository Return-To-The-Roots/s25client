#################################################################################
### $Id: CMakeLists.txt 7506 2011-09-07 13:16:08Z FloSoft $
#################################################################################

INCLUDE(FindSDL)

IF (SDL_FOUND)
	INCLUDE(CMakeMacroCorrectLib)
	
	CORRECT_LIB(SDL_LIBRARY SDL)

	INCLUDE_DIRECTORIES(../../../src)

	INCLUDE(AddFileDependencies)
	ADD_FILE_DEPENDENCIES(../../../../build_version.cpp ${CMAKE_BINARY_DIR}/build_version.h)

	ADD_LIBRARY(videoSDL SHARED SDL.cpp SDL.h main.h ../../../../build_version.cpp ../../../src/DriverInterface.cpp)

	ADD_FLAGS(CMAKE_C_FLAGS -I${SDL_INCLUDE_DIR})
	ADD_FLAGS(CMAKE_CXX_FLAGS -I${SDL_INCLUDE_DIR})

	TARGET_LINK_LIBRARIES(videoSDL videodrv ${SDL_LIBRARY})

	SET(SL_PREFIX ${CMAKE_SHARED_LIBRARY_PREFIX})
	IF ( "${COMPILEFOR}" STREQUAL "windows")
		ADD_DEFINITIONS(-DBUILD_DLL)
		SET(SL_PREFIX "")
	ENDIF ( "${COMPILEFOR}" STREQUAL "windows")

	ADD_CUSTOM_COMMAND(TARGET videoSDL POST_BUILD COMMAND ln -fvs SDL/src/${SL_PREFIX}videoSDL${CMAKE_SHARED_LIBRARY_SUFFIX} ../../videoSDL${CMAKE_SHARED_LIBRARY_SUFFIX})
	
	INSTALL(TARGETS videoSDL 
		RUNTIME DESTINATION ${LIBDIR}/driver/video 
		LIBRARY DESTINATION ${LIBDIR}/driver/video
		ARCHIVE DESTINATION ${LIBDIR}/driver/video
	)
ELSE (SDL_FOUND)
	MESSAGE(WARNING ": SDL library not found: Not building SDL videodriver")
ENDIF (SDL_FOUND)

#################################################################################
