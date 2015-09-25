# - Find miniupnpc
# Find the native miniupnpc includes and libraries
#
#  MINIUPNPC_INCLUDE_DIR - where to find miniupnpc.h, etc.
#  MINIUPNPC_LIBRARIES   - List of libraries when using libminiupnpc.
#  MINIUPNPC_FOUND       - True if libminiupnpc found.

IF(MINIUPNPC_INCLUDE_DIR)
	# Already in cache, be silent
	SET(MINIUPNPC_FIND_QUIETLY TRUE)
ENDIF(MINIUPNPC_INCLUDE_DIR)

FIND_PATH(MINIUPNPC_INCLUDE_DIR NAMES miniupnpc/miniupnpc.h PATHS
	${MINIUPNPC_DIR_SEARCH}/miniupnpc/include
	/usr/include
	/usr/local/include
)

FIND_LIBRARY(MINIUPNPC_LIBRARY NAMES miniupnpc PATHS
	${MINIUPNPC_DIR_SEARCH}/lib
	/usr/lib
	/usr/local/lib
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(miniupnpc DEFAULT_MSG MINIUPNPC_INCLUDE_DIR MINIUPNPC_LIBRARY)

IF(MINIUPNPC_FOUND)
	SET(MINIUPNPC_LIBRARIES ${MINIUPNPC_LIBRARY})
ELSE(MINIUPNPC_FOUND)
	SET(MINIUPNPC_LIBRARIES)
ENDIF(MINIUPNPC_FOUND)

MARK_AS_ADVANCED(MINIUPNPC_INCLUDE_DIR MINIUPNPC_LIBRARY)
