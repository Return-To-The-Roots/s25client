# - Find samplerate
# Find the native samplerate includes and libraries
#
#  SAMPLERATE_INCLUDE_DIR - where to find samplerate.h, etc.
#  SAMPLERATE_LIBRARIES   - List of libraries when using libsamplerate.
#  SAMPLERATE_FOUND       - True if libsamplerate found.

IF(SAMPLERATE_INCLUDE_DIR)
	# Already in cache, be silent
	SET(SAMPLERATE_FIND_QUIETLY TRUE)
ENDIF(SAMPLERATE_INCLUDE_DIR)

FIND_PATH(SAMPLERATE_INCLUDE_DIR NAMES samplerate.h PATHS 
	${SAMPLERATE_DIR_SEARCH}/include
	/usr/include
	/usr/local/include
	/opt/local/include
	/Library/Frameworks
)

FIND_LIBRARY(SAMPLERATE_LIBRARY NAMES samplerate PATHS 
	${SAMPLERATE_DIR_SEARCH}/lib 
	/usr/lib 
	/usr/local/lib 
	/opt/local/lib 
	/Library/Frameworks
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SAMPLERATE DEFAULT_MSG SAMPLERATE_INCLUDE_DIR SAMPLERATE_LIBRARY)

IF(SAMPLERATE_FOUND)
	SET(SAMPLERATE_LIBRARIES ${SAMPLERATE_LIBRARY})
ELSE(SAMPLERATE_FOUND)
	SET(SAMPLERATE_LIBRARIES)
ENDIF(SAMPLERATE_FOUND)

MARK_AS_ADVANCED(SAMPLERATE_INCLUDE_DIR SAMPLERATE_LIBRARY)
