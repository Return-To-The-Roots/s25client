# set compiler flags
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lnetwork -lexecinfo")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -g -std=c11 -D_BSD_SOURCE")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -g -std=c++11 -D_BSD_SOURCE")
