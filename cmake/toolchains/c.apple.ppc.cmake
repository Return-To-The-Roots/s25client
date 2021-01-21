set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_PROCESSOR powerpc)

set(CMAKE_OSX_ARCHITECTURES "ppc" CACHE STRING "OSX-Architectures")

# set compilers...
include("${CMAKE_CURRENT_LIST_DIR}/c.apple.common.cmake")
