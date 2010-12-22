################################################################################
### $Id: c.linux.common.cmake 6807 2010-10-18 14:12:04Z FloSoft $
################################################################################

IF ( NOT ${CMAKE_PREFIX_PATH} STREQUAL "" )
	# where is the target environment 
	SET(CMAKE_FIND_ROOT_PATH ${CMAKE_PREFIX_PATH})

	FORCE_ADD_FLAGS(CMAKE_C_FLAGS -isysroot ${CMAKE_PREFIX_PATH})
	FORCE_ADD_FLAGS(CMAKE_CXX_FLAGS -isysroot ${CMAKE_PREFIX_PATH})

	FORCE_ADD_FLAGS(CMAKE_EXE_LINKER_FLAGS -isysroot ${CMAKE_PREFIX_PATH})
	FORCE_ADD_FLAGS(CMAKE_MODULE_LINKER_FLAGS -isysroot ${CMAKE_PREFIX_PATH})
	FORCE_ADD_FLAGS(CMAKE_SHARED_LINKER_FLAGS -isysroot ${CMAKE_PREFIX_PATH})
ENDIF ( NOT ${CMAKE_PREFIX_PATH} STREQUAL "" )

# set compiler flags
FORCE_ADD_FLAGS(CMAKE_C_FLAGS -mtune=generic)
FORCE_ADD_FLAGS(CMAKE_CXX_FLAGS -mtune=generic)

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

INCLUDE(cmake/linux.common.cmake)
