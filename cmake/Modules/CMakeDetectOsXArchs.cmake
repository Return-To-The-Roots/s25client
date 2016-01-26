# Detects all supported OSX architectures by reading libSystem.B.dylib
# Sets the possible values in DETECTED_OSX_ARCHS
# If a NO{ARCH} variable is set, then {ARCH} will be ignored

MACRO(DetectOsXArchs)
	SET(LIB_SYSTEM_PATH "${CMAKE_OSX_SYSROOT}/usr/lib/libSystem.B.dylib")
	
	IF(EXISTS ${LIB_SYSTEM_PATH})
		IF(NOT "${CMAKE_LIPO}" OR "${CMAKE_LIPO}" STREQUAL "")
			# find lipo
			FIND_PROGRAM(LIPO NAMES apple-lipo lipo i686-apple-darwin10-lipo)
			SET(CMAKE_LIPO "${LIPO}" CACHE PATH "" FORCE)
		ENDIF()

		MESSAGE(STATUS "Checking ${LIB_SYSTEM_PATH} for possible architectures")

		# read supported platforms	
		EXECUTE_PROCESS(
			COMMAND ${CMAKE_LIPO} "-info" "${LIB_SYSTEM_PATH}"
			RESULT_VARIABLE LIPO_RESULT
			ERROR_VARIABLE LIPO_ERROR
			OUTPUT_VARIABLE LIPO_OUTPUT
		)
	ELSE()
		# Newer OSX have only a text file *.tbd instead of the dylib file
		SET(LIB_SYSTEM_PATH "${CMAKE_OSX_SYSROOT}/usr/lib/libSystem.B.tbd")
		IF(NOT EXISTS ${LIB_SYSTEM_PATH})
			MESSAGE(FATAL_ERROR "No libSystem.B.* found while trying to detect the architecture")
		ENDIF()
		EXECUTE_PROCESS(
			COMMAND "grep" "archs" "${LIB_SYSTEM_PATH}"
			RESULT_VARIABLE LIPO_RESULT
			ERROR_VARIABLE LIPO_ERROR
			OUTPUT_VARIABLE LIPO_OUTPUT
		)
	ENDIF()

	SET(DETECTED_OSX_ARCHS "")

	IF("${LIPO_OUTPUT}" MATCHES "x86_64" AND "${RTTR_NOx86_64}" STREQUAL "")
		SET(DETECTED_OSX_ARCHS ${DETECTED_OSX_ARCHS} x86_64)
	ENDIF()

	IF("${LIPO_OUTPUT}" MATCHES "i386" AND "${RTTR_NOi386}" STREQUAL "")
		SET(DETECTED_OSX_ARCHS ${DETECTED_OSX_ARCHS} i386)
	ENDIF()

	IF("${LIPO_OUTPUT}" MATCHES "i686" AND "${RTTR_NOi686}" STREQUAL "")
		SET(DETECTED_OSX_ARCHS ${DETECTED_OSX_ARCHS} i686)
	ENDIF()

	IF("${LIPO_OUTPUT}" MATCHES "ppc" AND "${RTTR_NOppc}" STREQUAL "")
		SET(DETECTED_OSX_ARCHS ${DETECTED_OSX_ARCHS} ppc)
	ENDIF()

	MESSAGE(STATUS "Possible architectures:${DETECTED_OSX_ARCHS}")
ENDMACRO(DetectOsXArchs)
