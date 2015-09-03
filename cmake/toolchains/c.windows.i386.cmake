# this one is important
SET(CMAKE_SYSTEM_NAME Windows)
SET(CMAKE_SYSTEM_PROCESSOR i686)

# specify the cross compiler
SET(CMAKE_C_COMPILER i686-pc-mingw32-gcc)
SET(CMAKE_CXX_COMPILER i686-pc-mingw32-g++)
SET(CMAKE_RC_COMPILER i686-pc-mingw32-windres)
SET(CMAKE_RANLIB i686-pc-mingw32-ranlib CACHE PATH "" FORCE)

# find AR program
FIND_PROGRAM(WINDOWS_AR NAMES i686-pc-mingw32-ar i686-mingw32-ar ar DOC "path to mingw's ar executable")
SET(CMAKE_AR "${WINDOWS_AR}" CACHE PATH "" FORCE)

# where is the target environment 
SET(CMAKE_PREFIX_PATH "/usr/i686-pc-mingw32")
SET(CMAKE_FIND_ROOT_PATH "${CMAKE_PREFIX_PATH}")
SET(ENV{SDLMIXER_DIR} "${CMAKE_PREFIX_PATH}")

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
