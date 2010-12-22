# - Find sndfile
# Find the native sndfile includes and libraries
#
#  SNDFILE_INCLUDE_DIR - where to find sndfile.h, etc.
#  SNDFILE_LIBRARIES   - List of libraries when using libsndfile.
#  SNDFILE_FOUND       - True if libsndfile found.

IF(SNDFILE_INCLUDE_DIR)
	# Already in cache, be silent
	SET(SNDFILE_FIND_QUIETLY TRUE)
ENDIF(SNDFILE_INCLUDE_DIR)

FIND_PATH(SNDFILE_INCLUDE_DIR NAMES sndfile.h PATHS 
	${SNDFILE_DIR_SEARCH}/include
	/usr/include
	/usr/local/include
	/opt/local/include
	/Library/Frameworks
)

FIND_LIBRARY(SNDFILE_LIBRARY NAMES sndfile PATHS 
	${SNDFILE_DIR_SEARCH}/lib
	/usr/lib
	/usr/local/lib
	/opt/local/lib
	/Library/Frameworks
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SNDFILE DEFAULT_MSG SNDFILE_INCLUDE_DIR SNDFILE_LIBRARY)

IF(SNDFILE_FOUND)
	SET(SNDFILE_LIBRARIES ${SNDFILE_LIBRARY})
ELSE(SNDFILE_FOUND)
	SET(SNDFILE_LIBRARIES)
ENDIF(SNDFILE_FOUND)

MARK_AS_ADVANCED(SNDFILE_INCLUDE_DIR SNDFILE_LIBRARY)
