SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_PROCESSOR i686)

if(NOT CMAKE_SYSTEM_VERSION)
    message(FATAL_ERROR "Need to set CMAKE_SYSTEM_VERSION to target version (uname -r) on cmd line")
endif()

# specify the cross compiler
set(COMPILER_PREFIX i686-pc-linux-gnu)
find_program(CMAKE_C_COMPILER NAMES ${COMPILER_PREFIX}-gcc)
find_program(CMAKE_CXX_COMPILER NAMES ${COMPILER_PREFIX}-g++)

# set search prefix
SET(CMAKE_FIND_ROOT_PATH "/usr/${COMPILER_PREFIX}")

# search for programs in the build host directories
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

SET(Boost_COMPILER "-gcc44")
