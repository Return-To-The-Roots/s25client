################################################################################
### $Id: c.linux.i386.cmake 6807 2010-10-18 14:12:04Z FloSoft $
################################################################################

# this one is important
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_PROCESSOR i686)

# specify the cross compiler
SET_CCACHE(CMAKE_C_COMPILER   i486-linux-gnu-gcc)
SET_CCACHE(CMAKE_CXX_COMPILER i486-linux-gnu-g++)
SET(CMAKE_RANLIB i486-linux-gnu-ranlib CACHE PATH "" FORCE)

# find AR program
FIND_PROGRAM(LINUX_AR NAMES i486-linux-gnu-ar i686-linux-gnu-ar ar DOC "path to linux's ar executable")
SET(CMAKE_AR "${LINUX_AR}" CACHE PATH "" FORCE)

# set compiler flags for i686
FORCE_ADD_FLAGS(CMAKE_C_FLAGS -malign-double)
FORCE_ADD_FLAGS(CMAKE_CXX_FLAGS -malign-double)

INCLUDE(cmake/c.linux.common.cmake)
