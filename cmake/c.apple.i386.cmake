################################################################################
### $Id: c.apple.i386.cmake 6807 2010-10-18 14:12:04Z FloSoft $
################################################################################

# this one is important
SET(CMAKE_SYSTEM_NAME Darwin)
SET(CMAKE_SYSTEM_PROCESSOR i386)

SET(CMAKE_OSX_ARCHITECTURES "i386" CACHE STRING "OSX-Architectures")

INCLUDE(cmake/c.apple.common.cmake)
