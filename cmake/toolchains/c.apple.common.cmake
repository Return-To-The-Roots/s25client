# This is a util script intended to be included by darwin toolchains
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR NOT CMAKE_SYSTEM_PROCESSOR)
    message(FATAL_ERROR "Cannot use this toolchain file directly. You have to set CMAKE_SYSTEM_* first!")
endif()

# specify the cross compiler
set(COMPILER_PREFIX i686-apple-darwin10)
find_program(CMAKE_C_COMPILER NAMES ${COMPILER_PREFIX}-gcc)
find_program(CMAKE_CXX_COMPILER NAMES ${COMPILER_PREFIX}-g++)

set(OSX_SDKS "/usr/lib/apple/SDKs/MacOSX10.5.sdk" "/usr/lib/apple/SDKs/MacOSX10.4u.sdk")
#list(OSX_SDKS INSERT 0 "/usr/lib/apple/SDKs/MacOSX10.6.sdk")

# set SDK (use newest first)
MESSAGE(STATUS "Getting SDK. Old deployment target: ${CMAKE_OSX_DEPLOYMENT_TARGET}. Old sysroot: ${CMAKE_OSX_SYSROOT}")
unset(CMAKE_OSX_DEPLOYMENT_TARGET)
unset(CMAKE_OSX_SYSROOT)
foreach(SDK IN LISTS OSX_SDKS)
    IF(EXISTS ${SDK})
        SET(CMAKE_OSX_SYSROOT ${SDK})
        break()
    endif()
endforeach()
if(NOT CMAKE_OSX_SYSROOT)
	MESSAGE(FATAL_ERROR "No OSX SDK found!")
ENDIF()
SET(CMAKE_OSX_SYSROOT ${CMAKE_OSX_SYSROOT} CACHE PATH "Path to OSX SDK")
MESSAGE(STATUS "Using OSX SDK at ${CMAKE_OSX_SYSROOT}")

SET(CMAKE_FIND_ROOT_PATH ${CMAKE_OSX_SYSROOT})

# search for programs in the build host directories
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
