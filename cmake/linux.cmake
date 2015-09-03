# set compiler flags
FORCE_ADD_FLAGS(CMAKE_C_FLAGS -ffast-math -mmmx -msse -mfpmath=sse -ggdb)
FORCE_ADD_FLAGS(CMAKE_CXX_FLAGS -ffast-math -mmmx -msse -mfpmath=sse -ggdb)

IF(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "i686")
    FORCE_ADD_FLAGS(CMAKE_C_FLAGS -malign-double)
    FORCE_ADD_FLAGS(CMAKE_CXX_FLAGS -malign-double)
ENDIF()
