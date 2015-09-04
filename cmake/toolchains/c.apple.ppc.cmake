# this one is important
SET(CMAKE_SYSTEM_NAME Darwin)
SET(CMAKE_SYSTEM_PROCESSOR powerpc)

SET(CMAKE_OSX_ARCHITECTURES "ppc" CACHE STRING "OSX-Architectures")

# set compilers...
INCLUDE("${CMAKE_CURRENT_LIST_DIR}/c.apple.common.cmake")
