################################################################################
### $Id: CMakeLists.txt 7522 2011-09-08 20:57:56Z FloSoft $
################################################################################

IF ( NOT "${COMPILEFOR}" STREQUAL "windows" )
	CHECK_CXX_COMPILER_FLAG("-fPIC" FPIC_FLAGS)
	IF (FPIC_FLAGS)
		ADD_DEFINITIONS(-fPIC)
	ENDIF (FPIC_FLAGS)
ENDIF ( NOT "${COMPILEFOR}" STREQUAL "windows" )

################################################################################

INCLUDE(AddFileDependencies)
INCLUDE_DIRECTORIES("../../src")
ADD_FILE_DEPENDENCIES(../../build_version.cpp ${CMAKE_BINARY_DIR}/build_version.h)

ADD_LIBRARY(videodrv STATIC VideoDriver.cpp VideoDriver.h VideoInterface.cpp VideoInterface.h ../../build_version.cpp)
ADD_LIBRARY(audiodrv STATIC AudioDriver.cpp AudioDriver.h AudioInterface.cpp AudioInterface.h Sound.cpp Sound.h Interface.h ../../build_version.cpp)

################################################################################
