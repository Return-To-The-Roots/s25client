FIND_PACKAGE(Boost 1.55.0 REQUIRED)
IF ( NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows" )
    INCLUDE(CheckCXXCompilerFlag)
	CHECK_CXX_COMPILER_FLAG(-fPIC FLAG_FPIC_SUPPORTED)
	IF(FLAG_FPIC_SUPPORTED)
		ADD_DEFINITIONS(-fPIC)
	ENDIF()
ENDIF()

################################################################################

INCLUDE(AddFileDependencies)
INCLUDE_DIRECTORIES("../../src" ${Boost_INCLUDE_DIR})
ADD_FILE_DEPENDENCIES(${PROJECT_SOURCE_DIR}/build_version.cpp ${CMAKE_BINARY_DIR}/build_version_defines.h)

ADD_LIBRARY(videodrv STATIC VideoDriver.cpp VideoDriver.h VideoInterface.cpp VideoInterface.h ${PROJECT_SOURCE_DIR}/build_version.cpp)
ADD_LIBRARY(audiodrv STATIC AudioDriver.cpp AudioDriver.h AudioInterface.cpp AudioInterface.h Sound.cpp Sound.h Interface.h ${PROJECT_SOURCE_DIR}/build_version.cpp)
ADD_DEPENDENCIES(videodrv updateversion)
ADD_DEPENDENCIES(audiodrv updateversion)

################################################################################
