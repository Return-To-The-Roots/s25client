# this one is important
SET(CMAKE_SYSTEM_NAME Darwin)
#SET(CMAKE_SYSTEM_PROCESSOR universal)

#SET(CMAKE_OSX_ARCHITECTURES "???" CACHE STRING "OSX-Architectures" FORCE)

# set compilers...
INCLUDE(cmake/toolchains/c.apple.common.cmake)
