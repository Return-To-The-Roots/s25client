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
IF(RTTR_ENABLE_OPTIMIZATIONS)
	IF (PLATFORM_ARCH STREQUAL "armv7l")
		FORCE_ADD_FLAGS(CMAKE_C_FLAGS -mcpu=cortex-a53 -mfloat-abi=hard -mfpu=neon-fp-armv8 -mneon-for-64bits -mtune=cortex-a53)
		FORCE_ADD_FLAGS(CMAKE_CXX_FLAGS -mcpu=cortex-a53 -mfloat-abi=hard -mfpu=neon-fp-armv8 -mneon-for-64bits -mtune=cortex-a53)
	ELSE ()
		FORCE_ADD_FLAGS(CMAKE_C_FLAGS -ffast-math -mmmx -msse -mfpmath=sse -ggdb)
		FORCE_ADD_FLAGS(CMAKE_CXX_FLAGS -ffast-math -mmmx -msse -mfpmath=sse -ggdb)
	ENDIF (PLATFORM_ARCH STREQUAL "armv7l")
ENDIF(RTTR_ENABLE_OPTIMIZATIONS)
