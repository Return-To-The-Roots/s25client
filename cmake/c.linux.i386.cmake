################################################################################
### $Id: c.linux.i386.cmake 7170 2011-04-15 18:19:47Z FloSoft $
################################################################################

# this one is important
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_PROCESSOR i686)

# specify the cross compiler
SET_CCACHE(CMAKE_C_COMPILER   i686-pc-linux-gnu-gcc)
SET_CCACHE(CMAKE_CXX_COMPILER i686-pc-linux-gnu-g++)

# find ranlib program
SET(CMAKE_RANLIB i686-pc-linux-gnu-ranlib CACHE PATH "" FORCE)

# find ar program
SET(CMAKE_AR i686-pc-linux-gnu-ar CACHE PATH "" FORCE)

# set compiler flags for i686
FORCE_ADD_FLAGS(CMAKE_C_FLAGS -malign-double)
FORCE_ADD_FLAGS(CMAKE_CXX_FLAGS -malign-double)

# set search prefix
SET(CMAKE_PREFIX_PATH "/usr/i686-pc-linux-gnu")

INCLUDE(cmake/c.linux.common.cmake)
