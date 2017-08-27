FIND_PACKAGE(Boost 1.55.0 REQUIRED)

################################################################################

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})

ADD_LIBRARY(videodrv STATIC VideoDriver.cpp VideoDriver.h VideoInterface.h)
ADD_LIBRARY(audiodrv STATIC AudioDriver.cpp AudioDriver.h AudioInterface.h Sound.cpp Sound.h)
SET_PROPERTY(TARGET videodrv PROPERTY POSITION_INDEPENDENT_CODE ON)
SET_PROPERTY(TARGET audiodrv PROPERTY POSITION_INDEPENDENT_CODE ON)

################################################################################
