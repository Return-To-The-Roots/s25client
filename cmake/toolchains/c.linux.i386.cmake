# this one is important
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_PROCESSOR i686)

# specify the cross compiler
SET_CCACHE(CMAKE_C_COMPILER   i686-pc-linux-gnu-gcc)
SET_CCACHE(CMAKE_CXX_COMPILER i686-pc-linux-gnu-g++)
SET(CMAKE_RANLIB i686-pc-linux-gnu-ranlib CACHE PATH "" FORCE)
SET(CMAKE_AR i686-pc-linux-gnu-ar CACHE PATH "" FORCE)

# set search prefix
SET(CMAKE_PREFIX_PATH "/usr/i686-pc-linux-gnu")
SET(CMAKE_FIND_ROOT_PATH "${CMAKE_PREFIX_PATH}")

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
