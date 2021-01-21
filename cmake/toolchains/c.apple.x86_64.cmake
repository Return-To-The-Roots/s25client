set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "OSX-Architectures")

# set compilers...
include("${CMAKE_CURRENT_LIST_DIR}/c.apple.common.cmake")
