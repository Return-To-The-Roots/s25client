set(CMAKE_SYSTEM_NAME Haiku)

if(NOT CMAKE_SYSTEM_VERSION)
    message(FATAL_ERROR "Need to set CMAKE_SYSTEM_VERSION to target version (uname -r) on cmd line")
endif()
