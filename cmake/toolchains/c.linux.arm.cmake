# this one is important
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_PROCESSOR arm)

# specify the cross compiler
SET(CMAKE_C_COMPILER   arm-linux-gnueabi-gcc)
SET(CMAKE_CXX_COMPILER arm-linux-gnueabi-g++)
SET(CMAKE_RANLIB arm-linux-gnueabi-ranlib CACHE PATH "" FORCE)
SET(CMAKE_AR arm-linux-gnueabi-ar CACHE PATH "" FORCE)

# set search prefix
SET(CMAKE_PREFIX_PATH "/usr/arm-linux-gnueabi")
SET(CMAKE_FIND_ROOT_PATH "${CMAKE_PREFIX_PATH}")

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

