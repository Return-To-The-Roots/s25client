################################################################################
### $Id: c.windows.i386.cmake 6807 2010-10-18 14:12:04Z FloSoft $
################################################################################

# this one is important
SET(CMAKE_SYSTEM_NAME Windows)
SET(CMAKE_SYSTEM_PROCESSOR i686)

# specify the cross compiler
SET_CCACHE(CMAKE_C_COMPILER i586-mingw32msvc-gcc)
SET_CCACHE(CMAKE_CXX_COMPILER i586-mingw32msvc-g++)
SET(CMAKE_RANLIB i586-mingw32msvc-ranlib CACHE PATH "" FORCE)

# find AR program
FIND_PROGRAM(WINDOWS_AR NAMES i586-mingw32msvc-ar i686-mingw32-ar ar DOC "path to mingw's ar executable")
SET(CMAKE_AR "${WINDOWS_AR}" CACHE PATH "" FORCE)

# where is the target environment 
SET(CMAKE_PREFIX_PATH "/usr/i586-mingw32msvc")

INCLUDE(cmake/c.windows.common.cmake)
