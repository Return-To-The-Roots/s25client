FIND_PACKAGE(SDL)

IF (SDL_FOUND)
	FIND_PACKAGE(Boost 1.55.0 REQUIRED)
	INCLUDE(CMakeMacroCorrectLib)
	
	CORRECT_LIB(SDL_LIBRARY SDL)

	ADD_LIBRARY(videoSDL SHARED VideoSDL.cpp VideoSDL.h ${PROJECT_SOURCE_DIR}/driver/src/DriverInterface.cpp)
	TARGET_LINK_LIBRARIES(videoSDL videodrv s25util ${SDL_LIBRARY})
	target_include_directories(videoSDL PRIVATE ${SDL_INCLUDE_DIR} ${Boost_INCLUDE_DIR})

	IF (WIN32)
		target_compile_definitions(videoSDL PRIVATE -DBUILD_DLL)
        find_package(OpenGL REQUIRED)
        target_link_libraries(videoSDL ${OPENGL_gl_LIBRARY}) 
        target_include_directories(videoSDL PRIVATE ${OPENGL_INCLUDE_DIR})
        include(GatherDll)
        gather_dll_by_name(SDL SDL.dll)
	ENDIF()

	INSTALL(TARGETS videoSDL DESTINATION ${RTTR_DRIVERDIR}/video)
ELSE (SDL_FOUND)
	MESSAGE(WARNING ": SDL library not found: Not building SDL videodriver")
ENDIF (SDL_FOUND)

#################################################################################
