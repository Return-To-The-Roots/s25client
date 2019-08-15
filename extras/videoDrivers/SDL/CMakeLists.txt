set(SDL_BUILDING_LIBRARY ON)
FIND_PACKAGE(SDL)

IF (SDL_FOUND)
	FIND_PACKAGE(Boost 1.64.0 REQUIRED)
	include(CorrectLib)	
	CORRECT_LIB(SDL_LIBRARY SDL)

	ADD_LIBRARY(videoSDL SHARED ${RTTR_DRIVER_INTERFACE} VideoSDL.cpp VideoSDL.h)
	target_link_libraries(videoSDL PRIVATE videodrv ${SDL_LIBRARY} nowide::static)
	target_include_directories(videoSDL PRIVATE ${SDL_INCLUDE_DIR})

	IF (WIN32)
        find_package(OpenGL REQUIRED)
        target_link_libraries(videoSDL PRIVATE ${OPENGL_gl_LIBRARY}) 
        target_include_directories(videoSDL PRIVATE ${OPENGL_INCLUDE_DIR} ${PROJECT_SOURCE_DIR}/data/win32)
        include(GatherDll)
        gather_dll_by_name(SDL SDL.dll)
	ENDIF()

	INSTALL(TARGETS videoSDL
		RUNTIME DESTINATION ${RTTR_DRIVERDIR}/video
		LIBRARY DESTINATION ${RTTR_DRIVERDIR}/video
	)
	add_dependencies(drivers videoSDL)
ELSE ()
	MESSAGE(WARNING ": SDL library not found: Not building SDL videodriver")
ENDIF ()
