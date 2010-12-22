#################################################################################
### $Id: CMakeLists.txt 6458 2010-05-31 11:38:51Z FloSoft $
#################################################################################

IF ("${COMPILEFOR}/${COMPILEARCH}" STREQUAL "linux/i386" )
	FIND_LIBRARY(GLFW_LIBRARY glfw) 
	
	IF (GLFW_LIBRARY)
		INCLUDE_DIRECTORIES(../../../src)

		INCLUDE(AddFileDependencies)
		ADD_FILE_DEPENDENCIES(../../../../build_version.cpp ${CMAKE_BINARY_DIR}/build_version.h)

		ADD_LIBRARY(videoGLFW SHARED GLFW.cpp GLFW.h oem.cpp oem.h main.h ../../../../build_version.cpp ../../../src/DriverInterface.cpp)
	
		TARGET_LINK_LIBRARIES(videoGLFW videodrv ${GLFW_LIBRARY})
	
		ADD_CUSTOM_COMMAND(TARGET videoGLFW POST_BUILD COMMAND ln -fvs GLFW/src/${CMAKE_SHARED_LIBRARY_PREFIX}videoGLFW${CMAKE_SHARED_LIBRARY_SUFFIX} ../../videoGLFW${CMAKE_SHARED_LIBRARY_SUFFIX})
	
		INSTALL(TARGETS videoGLFW DESTINATION ${LIBDIR}/driver/video)
	ELSE (GLFW_LIBRARY)
		MESSAGE(WARNING ": GLFW library not found: Not building GLFW videodriver")
	ENDIF (GLFW_LIBRARY)
ENDIF ("${COMPILEFOR}/${COMPILEARCH}" STREQUAL "linux/i386" )

#################################################################################
