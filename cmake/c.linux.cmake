SET(Boost_COMPILER "-gcc44")

# set compiler flags
IF(ENABLE_OPTIMIZATIONS)
	FORCE_ADD_FLAGS(CMAKE_C_FLAGS -mtune=generic -mno-align-double)
	FORCE_ADD_FLAGS(CMAKE_CXX_FLAGS -mtune=generic -mno-align-double)
ENDIF(ENABLE_OPTIMIZATIONS)
