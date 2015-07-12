#################################################################################
### $Id: CMakeLists.txt 7506 2011-09-07 13:16:08Z FloSoft $
#################################################################################

FIND_PACKAGE(SDL)

IF (SDL_FOUND)
	INCLUDE(CMakeMacroCorrectLib)
	
	CORRECT_LIB(SDL_LIBRARY SDL)

	INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/driver/src ${SDL_INCLUDE_DIR})

	INCLUDE(AddFileDependencies)
	ADD_FILE_DEPENDENCIES(${PROJECT_SOURCE_DIR}/build_version.cpp ${CMAKE_BINARY_DIR}/build_version.h)

	ADD_LIBRARY(videoSDL SHARED SDL.cpp SDL.h main.h ${PROJECT_SOURCE_DIR}/build_version.cpp ${PROJECT_SOURCE_DIR}/driver/src/DriverInterface.cpp)
	ADD_DEPENDENCIES(videoSDL updateversion)

	TARGET_LINK_LIBRARIES(videoSDL videodrv ${SDL_LIBRARY})

	SET(SL_PREFIX ${CMAKE_SHARED_LIBRARY_PREFIX})
	IF ( "${COMPILEFOR}" STREQUAL "windows")
		ADD_DEFINITIONS(-DBUILD_DLL)
		SET(SL_PREFIX "")
	ENDIF ( "${COMPILEFOR}" STREQUAL "windows")

	IF (MSVC)
		SET_TARGET_PROPERTIES(videoSDL PROPERTIES
			RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}/../.."
			RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}/../.."
		)
	ELSE()
		ADD_CUSTOM_COMMAND(TARGET videoSDL POST_BUILD COMMAND ln -fvs SDL/src/${SL_PREFIX}videoSDL${CMAKE_SHARED_LIBRARY_SUFFIX} ../../videoSDL${CMAKE_SHARED_LIBRARY_SUFFIX})
	ENDIF()
	
	INSTALL(TARGETS videoSDL 
		RUNTIME DESTINATION ${LIBDIR}/driver/video 
		LIBRARY DESTINATION ${LIBDIR}/driver/video
		ARCHIVE DESTINATION ${LIBDIR}/driver/video
	)
ELSE (SDL_FOUND)
	MESSAGE(WARNING ": SDL library not found: Not building SDL videodriver")
ENDIF (SDL_FOUND)

#################################################################################
