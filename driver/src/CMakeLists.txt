FIND_PACKAGE(Boost 1.55.0 REQUIRED)

################################################################################

INCLUDE(AddFileDependencies)
INCLUDE_DIRECTORIES("../../src" ${Boost_INCLUDE_DIR})
ADD_FILE_DEPENDENCIES(${PROJECT_SOURCE_DIR}/build_version.cpp ${CMAKE_BINARY_DIR}/build_version_defines.h)

ADD_LIBRARY(videodrv STATIC VideoDriver.cpp VideoDriver.h VideoInterface.h ${PROJECT_SOURCE_DIR}/build_version.cpp)
ADD_LIBRARY(audiodrv STATIC AudioDriver.cpp AudioDriver.h AudioInterface.h Sound.cpp Sound.h Interface.h ${PROJECT_SOURCE_DIR}/build_version.cpp)
SET_PROPERTY(TARGET videodrv PROPERTY POSITION_INDEPENDENT_CODE ON)
SET_PROPERTY(TARGET audiodrv PROPERTY POSITION_INDEPENDENT_CODE ON)
ADD_DEPENDENCIES(videodrv updateversion)
ADD_DEPENDENCIES(audiodrv updateversion)

################################################################################
