set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_PROCESSOR i386)

set(CMAKE_OSX_ARCHITECTURES "i386" CACHE STRING "OSX-Architectures")

# set compilers...
include("${CMAKE_CURRENT_LIST_DIR}/c.apple.common.cmake")
