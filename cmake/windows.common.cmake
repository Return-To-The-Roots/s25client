################################################################################
### $Id: windows.common.cmake 7918 2012-04-01 12:58:20Z marcus $
################################################################################

# set compiler flags
FORCE_ADD_FLAGS(CMAKE_C_FLAGS -malign-double -ffast-math -mmmx -msse -mfpmath=sse -fomit-frame-pointer -ggdb)
FORCE_ADD_FLAGS(CMAKE_CXX_FLAGS -malign-double -ffast-math -mmmx -msse -mfpmath=sse -fomit-frame-pointer -ggdb)

# bugfix for cygwin
#ADD_DEFINITIONS(-D_WIN32 -D__USE_W32_SOCKETS)

FORCE_ADD_FLAGS(CMAKE_C_FLAGS -D_WIN32 -D__USE_W32_SOCKETS -DNOMINMAX)
FORCE_ADD_FLAGS(CMAKE_CXX_FLAGS -D_WIN32 -D__USE_W32_SOCKETS -DNOMINMAX)
