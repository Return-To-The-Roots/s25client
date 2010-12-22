################################################################################
### $Id: CMakeLists.txt 6458 2010-05-31 11:38:51Z FloSoft $
################################################################################

IF ( NOT "${COMPILEFOR}" STREQUAL "windows" )
	IF ("${CROSSCOMPILE}" STREQUAL "1" OR "${COMPILEARCH}" STREQUAL "x86_64" )
		ADD_DEFINITIONS(-fPIC)
	ENDIF ("${CROSSCOMPILE}" STREQUAL "1" OR "${COMPILEARCH}" STREQUAL "x86_64" )
ENDIF ( NOT "${COMPILEFOR}" STREQUAL "windows" )

################################################################################

INCLUDE(AddFileDependencies)
ADD_FILE_DEPENDENCIES(../../build_version.cpp ${CMAKE_BINARY_DIR}/build_version.h)

ADD_LIBRARY(videodrv STATIC VideoDriver.cpp VideoDriver.h VideoInterface.cpp VideoInterface.h ../../build_version.cpp)
ADD_LIBRARY(audiodrv STATIC AudioDriver.cpp AudioDriver.h AudioInterface.cpp AudioInterface.h Sound.cpp Sound.h Interface.h ../../build_version.cpp)

################################################################################
