FIND_PACKAGE(SDL)

IF (SDL_FOUND)
	FIND_PACKAGE(Boost 1.55.0 REQUIRED)
	INCLUDE(CMakeMacroCorrectLib)
	
	CORRECT_LIB(SDL_LIBRARY SDL)

	INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR} ${Boost_INCLUDE_DIR})

	IF (WIN32)
		ADD_DEFINITIONS(-DBUILD_DLL)
        include(GatherDll)
        gather_dll_by_name(SDL SDL.dll)
	ENDIF ()

	ADD_LIBRARY(videoSDL SHARED VideoSDL.cpp VideoSDL.h ${PROJECT_SOURCE_DIR}/driver/src/DriverInterface.cpp ${COMMON_SRC})
	TARGET_LINK_LIBRARIES(videoSDL videodrv s25util ${SDL_LIBRARY})
	if(WIN32)
		target_link_libraries(videoSDL opengl32)
	endif()

	SET_PROPERTY(TARGET videoSDL PROPERTY POSITION_INDEPENDENT_CODE ON)

	INSTALL(TARGETS videoSDL DESTINATION ${RTTR_DRIVERDIR}/video)
ELSE (SDL_FOUND)
	MESSAGE(WARNING ": SDL library not found: Not building SDL videodriver")
ENDIF (SDL_FOUND)

#################################################################################
