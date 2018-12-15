set(CMAKE_SYSTEM_NAME Linux)

if(NOT CMAKE_SYSTEM_VERSION)
    message(FATAL_ERROR "Need to set CMAKE_SYSTEM_VERSION to target version (uname -r) on cmd line")
endif()

find_program(CMAKE_C_COMPILER NAMES ${COMPILER_PREFIX}-gcc)
find_program(CMAKE_CXX_COMPILER NAMES ${COMPILER_PREFIX}-g++)
find_program(CMAKE_LDD NAMES ${COMPILER_PREFIX}-ldd ldd)

include(${CMAKE_CURRENT_LIST_DIR}/CreateBundleUtilSymlinks.cmake)
create_bundle_util_symlinks(${CMAKE_DUMPBIN} ${CMAKE_OBJDUMP})

# set search prefix
set(CMAKE_FIND_ROOT_PATH "/usr/${COMPILER_PREFIX}")

# search for programs in the build host directories
# for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
