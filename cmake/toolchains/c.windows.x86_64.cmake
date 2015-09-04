INCLUDE("${CMAKE_CURRENT_LIST_DIR}/../Modules/CMakeMacroSetCCache.cmake")

# this one is important
SET(CMAKE_SYSTEM_NAME Windows)
SET(CMAKE_SYSTEM_PROCESSOR x86_64)

# specify the cross compiler
SET_CCACHE(CMAKE_C_COMPILER x86_64-pc-mingw32-gcc)
SET_CCACHE(CMAKE_CXX_COMPILER x86_64-pc-mingw32-g++)
SET_CCACHE(CMAKE_RC_COMPILER x86_64-pc-mingw32-windres)
SET(CMAKE_RANLIB x86_64-pc-mingw32-ranlib CACHE PATH "" FORCE)

# find AR program
FIND_PROGRAM(WINDOWS_AR NAMES x86_64-pc-mingw32-ar x86_64-mingw32-ar ar DOC "path to mingw's ar executable")
SET(CMAKE_AR "${WINDOWS_AR}" CACHE PATH "" FORCE)

# where is the target environment 
SET(CMAKE_PREFIX_PATH "/usr/x86_64-pc-mingw32")
SET(CMAKE_FIND_ROOT_PATH "${CMAKE_PREFIX_PATH}")
SET(ENV{SDLMIXER_DIR} "${CMAKE_PREFIX_PATH}")

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
