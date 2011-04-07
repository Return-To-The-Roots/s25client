################################################################################
### $Id: c.apple.common.cmake 7125 2011-04-07 15:03:44Z FloSoft $
################################################################################

# specify the cross compiler
SET_CCACHE(CMAKE_C_COMPILER i686-apple-darwin10-gcc)
SET_CCACHE(CMAKE_CXX_COMPILER i686-apple-darwin10-g++)
SET(CMAKE_RANLIB i686-apple-darwin10-ranlib CACHE STRING "" FORCE)
SET(CMAKE_LIPO i686-apple-darwin10-lipo CACHE STRING "" FORCE)

# set OSX-Version
SET(CMAKE_OSX_DEPLOYMENT_TARGET "10.4" CACHE STRING "OSX-Target")

# set SDK
SET(CMAKE_PREFIX_PATH "/usr/lib/apple/SDKs/MacOSX10.5.sdk")

FORCE_ADD_FLAGS(CMAKE_C_FLAGS -isysroot ${CMAKE_PREFIX_PATH})
FORCE_ADD_FLAGS(CMAKE_CXX_FLAGS -isysroot ${CMAKE_PREFIX_PATH})

FORCE_ADD_FLAGS(CMAKE_EXE_LINKER_FLAGS -isysroot ${CMAKE_PREFIX_PATH})
FORCE_ADD_FLAGS(CMAKE_MODULE_LINKER_FLAGS -isysroot ${CMAKE_PREFIX_PATH})
FORCE_ADD_FLAGS(CMAKE_SHARED_LINKER_FLAGS -isysroot ${CMAKE_PREFIX_PATH})

# disable optimization - buggy gcc?
FORCE_ADD_FLAGS(CMAKE_CXX_FLAGS_MINSIZEREL -O0 -DNDEBUG)
FORCE_ADD_FLAGS(CMAKE_CXX_FLAGS_RELEASE -O0 -DNDEBUG)
FORCE_ADD_FLAGS(CMAKE_CXX_FLAGS_RELWITHDEBINFO -O0 -g)

# set root path
SET(CMAKE_FIND_ROOT_PATH "${CMAKE_PREFIX_PATH}")

INCLUDE(cmake/apple.common.cmake)
