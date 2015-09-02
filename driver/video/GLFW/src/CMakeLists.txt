
IF ("${COMPILEFOR}/${COMPILEARCH}" STREQUAL "linux/i386" )
	FIND_LIBRARY(GLFW_LIBRARY glfw) 
	
	IF (GLFW_LIBRARY)
		INCLUDE_DIRECTORIES(../../../src)

		INCLUDE(AddFileDependencies)
		ADD_FILE_DEPENDENCIES(${PROJECT_SOURCE_DIR}/build_version.cpp ${CMAKE_BINARY_DIR}/build_version.h)

		ADD_LIBRARY(videoGLFW SHARED GLFW.cpp GLFW.h oem.cpp oem.h main.h .${PROJECT_SOURCE_DIR}/build_version.cpp ../../../src/DriverInterface.cpp)
		ADD_DEPENDENCIES(videoGLFW updateversion)
	
		TARGET_LINK_LIBRARIES(videoGLFW videodrv ${GLFW_LIBRARY})
	
		IF (MSVC)
			SET_TARGET_PROPERTIES(videoGLFW PROPERTIES
				RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}/../.."
				RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}/../.."
			)
		ELSE()
			ADD_CUSTOM_COMMAND(TARGET videoGLFW POST_BUILD COMMAND ln -fvs GLFW/src/${CMAKE_SHARED_LIBRARY_PREFIX}videoGLFW${CMAKE_SHARED_LIBRARY_SUFFIX} ../../videoGLFW${CMAKE_SHARED_LIBRARY_SUFFIX})
		ENDIF()
	
		INSTALL(TARGETS videoGLFW DESTINATION ${LIBDIR}/driver/video)
	ELSE (GLFW_LIBRARY)
		MESSAGE(WARNING ": GLFW library not found: Not building GLFW videodriver")
	ENDIF (GLFW_LIBRARY)
ENDIF ("${COMPILEFOR}/${COMPILEARCH}" STREQUAL "linux/i386" )

#################################################################################
