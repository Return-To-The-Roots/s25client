SET(Boost_USE_STATIC_LIBS TRUE)

#Check for ccache
find_program(CCACHE_FOUND ccache)
MARK_AS_ADVANCED(CCACHE_FOUND)
if(CCACHE_FOUND)
	MESSAGE(STATUS "Using ccache to speed up builds")
	set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
	set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

# set compiler flags
FORCE_ADD_FLAGS(CMAKE_C_FLAGS -ffast-math -mmmx -msse -mfpmath=sse -ggdb)
FORCE_ADD_FLAGS(CMAKE_CXX_FLAGS -ffast-math -mmmx -msse -mfpmath=sse -ggdb)

