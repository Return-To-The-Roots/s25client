# specify the cross compiler
SET(CMAKE_C_COMPILER i686-apple-darwin10-gcc)
SET(CMAKE_CXX_COMPILER i686-apple-darwin10-g++)
SET(CMAKE_RANLIB i686-apple-darwin10-ranlib CACHE STRING "" FORCE)
SET(CMAKE_LIPO i686-apple-darwin10-lipo CACHE STRING "" FORCE)

# set OSX-Version
SET(CMAKE_OSX_DEPLOYMENT_TARGET "10.5" CACHE STRING "OSX-Target")

# set SDK
SET(OSX_SDK "/usr/lib/apple/SDKs/MacOSX10.5.sdk")
SET(CMAKE_PREFIX_PATH ${OSX_SDK})
SET(CMAKE_FIND_ROOT_PATH ${OSX_SDK})

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
