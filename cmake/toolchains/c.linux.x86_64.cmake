INCLUDE("${CMAKE_CURRENT_LIST_DIR}/../Modules/CMakeMacroSetCCache.cmake")

# this one is important
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_PROCESSOR x86_64)

# specify the cross compiler
SET_CCACHE(CMAKE_C_COMPILER   x86_64-pc-linux-gnu-gcc)
SET_CCACHE(CMAKE_CXX_COMPILER x86_64-pc-linux-gnu-g++)
SET(CMAKE_RANLIB x86_64-pc-linux-gnu-ranlib CACHE PATH "" FORCE)

# set search prefix
SET(CMAKE_PREFIX_PATH "/usr/x86_64-pc-linux-gnu")
SET(CMAKE_FIND_ROOT_PATH ${CMAKE_PREFIX_PATH})
SET(BOOST_ROOT ${CMAKE_PREFIX_PATH})

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
