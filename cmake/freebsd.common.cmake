# set compiler flags
FORCE_ADD_FLAGS(CMAKE_C_FLAGS -O2 -g -std=c11)
FORCE_ADD_FLAGS(CMAKE_CXX_FLAGS -O2 -g -std=c++11)

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
