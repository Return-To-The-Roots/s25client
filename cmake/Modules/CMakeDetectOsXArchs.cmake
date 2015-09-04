# Detects all supported OSX architectures by reading libSystem.B.dylib
# Sets the possible values in DETECTED_OSX_ARCHS
# If a NO{ARCH} variable is set, then {ARCH} will be ignored

MACRO(DetectOsXArchs)
    # find lipo
	FIND_PROGRAM(LIPO NAMES apple-lipo lipo)
	SET(CMAKE_LIPO "${LIPO}" CACHE PATH "" FORCE)

	MESSAGE(STATUS "Checking ${CMAKE_PREFIX_PATH}/usr/lib/libSystem.B.dylib for possible architectures")

	# read supported platforms	
	EXECUTE_PROCESS(
		COMMAND ${LIPO} "-info" "${CMAKE_PREFIX_PATH}/usr/lib/libSystem.B.dylib"
		RESULT_VARIABLE LIPO_RESULT
		ERROR_VARIABLE LIPO_ERROR
		OUTPUT_VARIABLE LIPO_OUTPUT
	)

	SET(DETECTED_OSX_ARCHS "")

	IF ( "${LIPO_OUTPUT}" MATCHES "x86_64" AND "${NOx86_64}" STREQUAL "" )
		SET(DETECTED_OSX_ARCHS ${DETECTED_OSX_ARCHS} x86_64)
	ENDIF ( "${LIPO_OUTPUT}" MATCHES "x86_64" AND "${NOx86_64}" STREQUAL "" )

	IF ( "${LIPO_OUTPUT}" MATCHES "i386" AND "${NOi386}" STREQUAL "" )
		SET(DETECTED_OSX_ARCHS ${DETECTED_OSX_ARCHS} i386)
	ENDIF ( "${LIPO_OUTPUT}" MATCHES "i386" AND "${NOi386}" STREQUAL "" )

	IF ( "${LIPO_OUTPUT}" MATCHES "ppc" AND "${NOppc}" STREQUAL "" )
		SET(DETECTED_OSX_ARCHS ${DETECTED_OSX_ARCHS} ppc)
	ENDIF ( "${LIPO_OUTPUT}" MATCHES "ppc" AND "${NOppc}" STREQUAL "" )

	MESSAGE(STATUS "Possible architectures:${DETECTED_OSX_ARCHS}")
ENDMACRO(DetectOsXArchs)
