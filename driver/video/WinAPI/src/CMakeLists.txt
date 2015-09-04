
IF ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows" )
	FIND_LIBRARY(GDI32_LIBRARY    gdi32)
	FIND_LIBRARY(OPENGL32_LIBRARY opengl32)
	FIND_LIBRARY(USER32_LIBRARY   user32)

	# bug: cmake cross detection's not implemented, so he never finds anything!
	
	#IF (GDI32_LIBRARY AND OPENGL_LIBRARY AND USER32_LIBRARY)
		INCLUDE_DIRECTORIES(../../../src)
	
		INCLUDE(AddFileDependencies)
		ADD_FILE_DEPENDENCIES(${PROJECT_SOURCE_DIR}/build_version.cpp ${CMAKE_BINARY_DIR}/build_version.h)

		ADD_LIBRARY(videoWinAPI SHARED WinAPI.cpp WinAPI.h main.h ${PROJECT_SOURCE_DIR}/build_version.cpp ../../../src/DriverInterface.cpp)
		ADD_DEPENDENCIES(videoWinAPI updateversion)
	
		TARGET_LINK_LIBRARIES(videoWinAPI videodrv gdi32 opengl32 user32) 
		# ${GDI32_LIBRARY} ${OPENGL32_LIBRARY} ${USER32_LIBRARY})
	
		ADD_DEFINITIONS(-DBUILD_DLL)
		SET(SL_PREFIX "")
		
		IF (MSVC)
			SET_TARGET_PROPERTIES(videoWinAPI PROPERTIES
				RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}/../.."
				RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}/../.."
			)
		ELSE()
			ADD_CUSTOM_COMMAND(TARGET videoWinAPI POST_BUILD COMMAND ln -fvs WinAPI/src/${SL_PREFIX}videoWinAPI${CMAKE_SHARED_LIBRARY_SUFFIX} ../../videoWinAPI${CMAKE_SHARED_LIBRARY_SUFFIX})
		ENDIF()

		INSTALL(TARGETS videoWinAPI RUNTIME DESTINATION ${LIBDIR}/driver/video)
	#ENDIF (GDI32_LIBRARY AND OPENGL_LIBRARY AND USER32_LIBRARY)
ENDIF ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows" )

#################################################################################
