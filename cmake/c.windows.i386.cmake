################################################################################
### $Id: c.windows.i386.cmake 7909 2012-03-31 12:26:40Z marcus $
################################################################################

# this one is important
SET(CMAKE_SYSTEM_NAME Windows)
SET(CMAKE_SYSTEM_PROCESSOR i686)

# specify the cross compiler
SET_CCACHE(CMAKE_C_COMPILER i686-pc-mingw32-gcc)
SET_CCACHE(CMAKE_CXX_COMPILER i686-pc-mingw32-g++)
SET_CCACHE(CMAKE_RC_COMPILER i686-pc-mingw32-windres)

SET(CMAKE_RANLIB i686-pc-mingw32-ranlib CACHE PATH "" FORCE)

# find AR program
FIND_PROGRAM(WINDOWS_AR NAMES i686-pc-mingw32-ar i686-mingw32-ar ar DOC "path to mingw's ar executable")
SET(CMAKE_AR "${WINDOWS_AR}" CACHE PATH "" FORCE)

# where is the target environment 
SET(CMAKE_PREFIX_PATH "/usr/i686-pc-mingw32")
SET(ENV{SDLMIXER_DIR} "${CMAKE_PREFIX_PATH}")

INCLUDE(cmake/c.windows.common.cmake)
