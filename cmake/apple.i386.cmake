################################################################################
### $Id: apple.i386.cmake 6807 2010-10-18 14:12:04Z FloSoft $
################################################################################

# this one is important
SET(CMAKE_SYSTEM_NAME Darwin)
SET(CMAKE_SYSTEM_PROCESSOR i386)

SET_CCACHE(CMAKE_C_COMPILER   gcc)
SET_CCACHE(CMAKE_CXX_COMPILER g++)

SET(COMPILEARCH "i386")
SET(CMAKE_OSX_ARCHITECTURES "${COMPILEARCH}")

INCLUDE(cmake/apple.common.cmake)

# disable cmake's own -arch parameter generation
SET(CMAKE_OSX_ARCHITECTURES "" CACHE STRING "" FORCE)

