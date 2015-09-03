# this one is important
SET(CMAKE_SYSTEM_NAME Darwin)
SET(CMAKE_SYSTEM_PROCESSOR i386)

SET(CMAKE_OSX_ARCHITECTURES "i386" CACHE STRING "OSX-Architectures")

# set compilers...
INCLUDE(cmake/toolchains/c.apple.common.cmake)
