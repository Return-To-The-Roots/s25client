FIND_PACKAGE(Boost 1.55.0 REQUIRED)

################################################################################

INCLUDE_DIRECTORIES(SYSTEM ${Boost_INCLUDE_DIR})

ADD_LIBRARY(videodrv STATIC VideoDriver.cpp ${DRIVER_INCLUDE_DIR}/VideoDriver.h ${DRIVER_INCLUDE_DIR}/VideoInterface.h)
ADD_LIBRARY(audiodrv STATIC AudioDriver.cpp ${DRIVER_INCLUDE_DIR}/AudioDriver.h ${DRIVER_INCLUDE_DIR}/AudioInterface.h ${DRIVER_INCLUDE_DIR}/SoundHandle.h)
SET_PROPERTY(TARGET videodrv PROPERTY POSITION_INDEPENDENT_CODE ON)
SET_PROPERTY(TARGET audiodrv PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(videodrv PUBLIC s25Common)
target_link_libraries(audiodrv PUBLIC s25Common)

################################################################################
