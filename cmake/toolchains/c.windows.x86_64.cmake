SET(CMAKE_SYSTEM_NAME Windows)
SET(CMAKE_SYSTEM_PROCESSOR x86_64)

# specify the cross compiler
set(COMPILER_PREFIX x86_64-pc-mingw32)
find_program(CMAKE_C_COMPILER NAMES ${COMPILER_PREFIX}-gcc)
find_program(CMAKE_CXX_COMPILER NAMES ${COMPILER_PREFIX}-g++)
find_program(CMAKE_RC_COMPILER NAMES ${COMPILER_PREFIX}-windres)

# set search prefix
SET(CMAKE_FIND_ROOT_PATH "/usr/${COMPILER_PREFIX}")
# CMake <= 3.0 workaround -.-
set(ENV{SDLDIR} ${CMAKE_FIND_ROOT_PATH})

# search for programs in the build host directories
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

SET(Boost_COMPILER "-mgw44")
