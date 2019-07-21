set(CMAKE_SYSTEM_NAME Haiku)

if(NOT CMAKE_SYSTEM_VERSION)
    message(FATAL_ERROR "Need to set CMAKE_SYSTEM_VERSION to target version (uname -r) on cmd line")
endif()

find_program(CMAKE_C_COMPILER NAMES ${COMPILER_PREFIX}-gcc)
find_program(CMAKE_CXX_COMPILER NAMES ${COMPILER_PREFIX}-g++)

