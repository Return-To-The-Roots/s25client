################################################################################
### $Id: c.apple.x86_64.cmake 6807 2010-10-18 14:12:04Z FloSoft $
################################################################################

# this one is important
SET(CMAKE_SYSTEM_NAME Darwin)
SET(CMAKE_SYSTEM_PROCESSOR x86_64)

SET(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "OSX-Architectures")

INCLUDE(cmake/c.apple.common.cmake)
