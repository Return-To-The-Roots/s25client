#################################################################################
### $Id: crosscompile.cmake 7115 2011-04-02 14:16:37Z FloSoft $
#################################################################################

# read host compiler machine triplet

IF(NOT COMPILEFOR_PLATFORM)
	SET(COMPILEFOR_PLATFORM "unknown")
ENDIF(NOT COMPILEFOR_PLATFORM)

MESSAGE(STATUS "Checking for ${COMPILEFOR_PLATFORM}")

INCLUDE(cmake/${COMPILEFOR_PLATFORM}.cmake OPTIONAL RESULT_VARIABLE FOUND_A)
IF(FOUND_A)
	MESSAGE(STATUS "Reading specifications from ${FOUND_A}")
ELSE(FOUND_A)
	INCLUDE(cmake/c.${COMPILEFOR_PLATFORM}.cmake OPTIONAL RESULT_VARIABLE FOUND_B)
	IF(FOUND_B)
	        MESSAGE(STATUS "Reading specifications from ${FOUND_B}")
	ENDIF(FOUND_B)
ENDIF(FOUND_A)

IF(NOT FOUND_A AND NOT FOUND_B)
	INCLUDE(cmake/${COMPILEFOR_PLATFORM}.local.cmake OPTIONAL RESULT_VARIABLE FOUND_C)
	IF(FOUND_C)
		MESSAGE(STATUS "Reading specifications from ${FOUND_C}")
	ENDIF(FOUND_C)
ENDIF(NOT FOUND_A AND NOT FOUND_B)

IF (NOT FOUND_A AND NOT FOUND_B AND NOT FOUND_C)
	MESSAGE(FATAL_ERROR " Platform specific include file(s) not found")
ENDIF (NOT FOUND_A AND NOT FOUND_B AND NOT FOUND_C)

#################################################################################

EXECUTE_PROCESS(
	COMMAND "gcc" "-dumpmachine"
	RESULT_VARIABLE HOST_GCC_RESULT
	ERROR_VARIABLE HOST_GCC_ERROR
	OUTPUT_VARIABLE HOST_GCC_OUTPUT
)
IF(NOT "${HOST_GCC_RESULT}" STREQUAL "0")
	MESSAGE(FATAL_ERROR "ERROR: gcc -dumpmachine failed... Result:'${HOST_GCC_RESULT}' Error:'${HOST_GCC_ERROR}' Output:'${HOST_GCC_OUTPUT}'")
ENDIF(NOT "${HOST_GCC_RESULT}" STREQUAL "0")

#################################################################################

# read target C compiler machine triplet

EXECUTE_PROCESS(
	COMMAND "${CMAKE_C_COMPILER}" "-dumpmachine"
	RESULT_VARIABLE USED_GCC_RESULT
	ERROR_VARIABLE USED_GCC_ERROR
	OUTPUT_VARIABLE USED_GCC_OUTPUT
)
IF(NOT "${USED_GCC_RESULT}" STREQUAL "0")
  MESSAGE(FATAL_ERROR "ERROR: ${CMAKE_C_COMPILER} -dumpmachine failed... Result:'${USED_GCC_RESULT}' Error:'${USED_GCC_ERROR}' Output:'${USED_GCC_OUTPUT}'")
ENDIF(NOT "${USED_GCC_RESULT}" STREQUAL "0")

#################################################################################

# read target C++ compiler machine triplet

EXECUTE_PROCESS(
	COMMAND "${CMAKE_CXX_COMPILER}" "-dumpmachine"
	RESULT_VARIABLE USED_GPP_RESULT
	ERROR_VARIABLE USED_GPP_ERROR
	OUTPUT_VARIABLE USED_GPP_OUTPUT
)
IF(NOT "${USED_GPP_RESULT}" STREQUAL "0")
	MESSAGE(FATAL_ERROR "ERROR: ${CMAKE_CXX_COMPILER} -dumpmachine failed... Result:'${USED_GPP_RESULT}' Error:'${USED_GPP_ERROR}' Output:'${USED_GPP_OUTPUT}'")
ENDIF(NOT "${USED_GPP_RESULT}" STREQUAL "0")

#################################################################################

# check if target compiler triplets match

IF(NOT "${USED_GCC_OUTPUT}" STREQUAL "${USED_GPP_OUTPUT}")
	MESSAGE(FATAL_ERROR "ERROR: Your C and C++ Compiler do not match: ${USED_GCC_OUTPUT} != $USED_GPP_OUTPUT}!")
ENDIF(NOT "${USED_GCC_OUTPUT}" STREQUAL "${USED_GPP_OUTPUT}")

#################################################################################

# strip newlines from var
STRING(REPLACE "\n" "" USED_GCC_OUTPUT ${USED_GCC_OUTPUT})
STRING(REPLACE "\n" "" HOST_GCC_OUTPUT ${HOST_GCC_OUTPUT})

#################################################################################

# do we crosscompile? if so, set a flag

IF (NOT "${HOST_GCC_OUTPUT}" STREQUAL "${USED_GCC_OUTPUT}")
	SET(CROSSCOMPILE "1")
	SET(CROSS "c.")
	MESSAGE(STATUS "Configuring for cross-compiling to ${USED_GCC_OUTPUT}")
ELSE (NOT "${HOST_GCC_OUTPUT}" STREQUAL "${USED_GCC_OUTPUT}")
	SET(CROSSCOMPILE "0")
	SET(CROSS "")
ENDIF (NOT "${HOST_GCC_OUTPUT}" STREQUAL "${USED_GCC_OUTPUT}")

#################################################################################

# Linux spezifische Parameter
IF ( "${USED_GCC_OUTPUT}" MATCHES "linux" )
	
	SET(COMPILEFOR "linux")
	
	IF ( "${USED_GCC_OUTPUT}" MATCHES "x86_64" )
		SET(COMPILEARCH "x86_64")
	ELSE ( "${USED_GCC_OUTPUT}" MATCHES "x86_64" )
		SET(COMPILEARCH "i386")
	ENDIF ( "${USED_GCC_OUTPUT}" MATCHES "x86_64" )
ENDIF ( "${USED_GCC_OUTPUT}" MATCHES "linux" )

#################################################################################

# Apple spezifische Parameter
IF ( "${USED_GCC_OUTPUT}" MATCHES "apple" )
	SET(COMPILEFOR "apple")

	# find lipo
	FIND_PROGRAM(LIPO NAMES ${USED_GCC_OUTPUT}-lipo lipo)
	SET(CMAKE_LIPO "${LIPO}" CACHE PATH "" FORCE)

	MESSAGE(STATUS "Checking ${CMAKE_PREFIX_PATH}/usr/lib/libSystem.B.dylib for possible architectures")

	# read supported platforms	
	EXECUTE_PROCESS(
		COMMAND ${LIPO} "-info" "${CMAKE_PREFIX_PATH}/usr/lib/libSystem.B.dylib"
		RESULT_VARIABLE LIPO_RESULT
		ERROR_VARIABLE LIPO_ERROR
		OUTPUT_VARIABLE LIPO_OUTPUT
	)

	IF("${COMPILEARCH}" STREQUAL "" OR "${COMPILEARCH}" STREQUAL "universal")	
		# always universal
		SET(COMPILEARCH "universal")
		SET(COMPILEARCHS "")
	
		IF ( "${LIPO_OUTPUT}" MATCHES "x86_64" AND "${NOx86_64}" STREQUAL "" )
			ADD_FLAGS(COMPILEARCHS x86_64)
		ENDIF ( "${LIPO_OUTPUT}" MATCHES "x86_64" AND "${NOx86_64}" STREQUAL "" )

		IF ( "${LIPO_OUTPUT}" MATCHES "i386" AND "${NOi386}" STREQUAL "" )
			ADD_FLAGS(COMPILEARCHS i386)
		ENDIF ( "${LIPO_OUTPUT}" MATCHES "i386" AND "${NOi386}" STREQUAL "" )

		IF ( "${LIPO_OUTPUT}" MATCHES "ppc" AND "${NOppc}" STREQUAL "" )
			ADD_FLAGS(COMPILEARCHS ppc)
		ENDIF ( "${LIPO_OUTPUT}" MATCHES "ppc" AND "${NOppc}" STREQUAL "" )
	ELSE("${COMPILEARCH}" STREQUAL "" OR "${COMPILEARCH}" STREQUAL "universal")
		SET(COMPILEARCHS "${COMPILEARCH}")
	ENDIF("${COMPILEARCH}" STREQUAL "" OR "${COMPILEARCH}" STREQUAL "universal")

	MESSAGE(STATUS "Possible architectures:${COMPILEARCHS}")	
ENDIF ( "${USED_GCC_OUTPUT}" MATCHES "apple" )

#################################################################################

# Cygwin/Mingw spezifische Parameter
IF ( "${USED_GCC_OUTPUT}" MATCHES "mingw" OR "${USED_GCC_OUTPUT}" MATCHES "cygwin" )
	SET(COMPILEFOR "windows")

    IF ( "${USED_GCC_OUTPUT}" MATCHES "x86_64" )
        SET(COMPILEARCH "x86_64")
    ELSE ( "${USED_GCC_OUTPUT}" MATCHES "x86_64" )
        SET(COMPILEARCH "i386")
    ENDIF ( "${USED_GCC_OUTPUT}" MATCHES "x86_64" )
ENDIF ( "${USED_GCC_OUTPUT}" MATCHES "mingw" OR "${USED_GCC_OUTPUT}" MATCHES "cygwin" )

#################################################################################

SET(CROSSCOMPILE "${CROSSCOMPILE}" CACHE INT "Are we cross compiling?")
SET(COMPILEFOR "${COMPILEFOR}" CACHE STRING "Target Platform")
SET(COMPILEARCH "${COMPILEARCH}" CACHE STRING "Target Architecture")
SET(COMPILEARCHS "${COMPILEARCHS}" CACHE STRING "Target Architectures")

#################################################################################

MESSAGE(STATUS "Compiling for ${COMPILEFOR}/${COMPILEARCH}")

INCLUDE(cmake/${CROSS}${COMPILEFOR}.${COMPILEARCH}.cmake OPTIONAL RESULT_VARIABLE FOUND_A)
IF(FOUND_A)
	MESSAGE(STATUS "Reading specifications from ${FOUND_A}")
ELSE(FOUND_A)
	INCLUDE(cmake/${CROSS}${COMPILEFOR}.cmake OPTIONAL RESULT_VARIABLE FOUND_B)
	IF(FOUND_B)
	        MESSAGE(STATUS "Reading specifications from ${FOUND_B}")
	ENDIF(FOUND_B)
ENDIF(FOUND_A)

IF (NOT FOUND_A AND NOT FOUND_B)
	INCLUDE(cmake/${CROSS}${COMPILEFOR}.local.cmake OPTIONAL RESULT_VARIABLE FOUND_C)
	IF(FOUND_C)
		MESSAGE(STATUS "Reading specifications from ${FOUND_C}")
	ENDIF(FOUND_C)
ENDIF (NOT FOUND_A AND NOT FOUND_B)

IF (NOT FOUND_A AND NOT FOUND_B AND NOT FOUND_C)
	MESSAGE(FATAL_ERROR " Architecture specific include file not found")
ENDIF (NOT FOUND_A AND NOT FOUND_B AND NOT FOUND_C)

#################################################################################
