################################################################################
### $Id: c.apple.universal.cmake 6807 2010-10-18 14:12:04Z FloSoft $
################################################################################

# this one is important
SET(CMAKE_SYSTEM_NAME Darwin)
#SET(CMAKE_SYSTEM_PROCESSOR universal)

SET(CMAKE_OSX_ARCHITECTURES "${COMPILEARCHS}" CACHE STRING "OSX-Architectures" FORCE)

INCLUDE(cmake/c.apple.common.cmake)
