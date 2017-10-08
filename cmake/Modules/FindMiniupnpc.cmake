# - Find miniupnpc
# Find the native miniupnpc includes and libraries
#
#  MINIUPNPC_INCLUDE_DIR - where to find miniupnpc.h, etc.
#  MINIUPNPC_LIBRARY     - Library to link to libminiupnpc.
#  MINIUPNPC_FOUND       - True if libminiupnpc found.

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
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Miniupnpc DEFAULT_MSG MINIUPNPC_INCLUDE_DIR MINIUPNPC_LIBRARY)

MARK_AS_ADVANCED(MINIUPNPC_INCLUDE_DIR MINIUPNPC_LIBRARY)
