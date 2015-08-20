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
	COMMAND "cc" "-dumpmachine"
	RESULT_VARIABLE HOST_CC_RESULT
	ERROR_VARIABLE HOST_CC_ERROR
	OUTPUT_VARIABLE HOST_CC_OUTPUT
)
IF(NOT "${HOST_CC_RESULT}" STREQUAL "0")
	MESSAGE(FATAL_ERROR "ERROR: cc -dumpmachine failed... Result:'${HOST_CC_RESULT}' Error:'${HOST_CC_ERROR}' Output:'${HOST_CC_OUTPUT}'")
ENDIF(NOT "${HOST_CC_RESULT}" STREQUAL "0")

#################################################################################

# read target C compiler machine triplet

EXECUTE_PROCESS(
	COMMAND "${CMAKE_C_COMPILER}" "-dumpmachine"
	RESULT_VARIABLE USED_CC_RESULT
	ERROR_VARIABLE USED_CC_ERROR
	OUTPUT_VARIABLE USED_CC_OUTPUT
)
IF(NOT "${USED_CC_RESULT}" STREQUAL "0")
  MESSAGE(FATAL_ERROR "ERROR: ${CMAKE_C_COMPILER} -dumpmachine failed... Result:'${USED_CC_RESULT}' Error:'${USED_CC_ERROR}' Output:'${USED_CC_OUTPUT}'")
ENDIF(NOT "${USED_CC_RESULT}" STREQUAL "0")

#################################################################################

# read target C++ compiler machine triplet

EXECUTE_PROCESS(
	COMMAND "${CMAKE_CXX_COMPILER}" "-dumpmachine"
	RESULT_VARIABLE USED_CPP_RESULT
	ERROR_VARIABLE USED_CPP_ERROR
	OUTPUT_VARIABLE USED_CPP_OUTPUT
)
IF(NOT "${USED_CPP_RESULT}" STREQUAL "0")
	MESSAGE(FATAL_ERROR "ERROR: ${CMAKE_CXX_COMPILER} -dumpmachine failed... Result:'${USED_CPP_RESULT}' Error:'${USED_CPP_ERROR}' Output:'${USED_CPP_OUTPUT}'")
ENDIF(NOT "${USED_CPP_RESULT}" STREQUAL "0")

#################################################################################

# check if target compiler triplets match

IF(NOT "${USED_CC_OUTPUT}" STREQUAL "${USED_CPP_OUTPUT}")
	MESSAGE(FATAL_ERROR "ERROR: Your C and C++ Compiler do not match: ${USED_CC_OUTPUT} != $USED_CPP_OUTPUT}!")
ENDIF(NOT "${USED_CC_OUTPUT}" STREQUAL "${USED_CPP_OUTPUT}")

#################################################################################

# strip newlines from var
STRING(REPLACE "\n" "" USED_CC_OUTPUT ${USED_CC_OUTPUT})
STRING(REPLACE "\n" "" HOST_CC_OUTPUT ${HOST_CC_OUTPUT})

#################################################################################

# do we crosscompile? if so, set a flag

IF (NOT "${HOST_CC_OUTPUT}" STREQUAL "${USED_CC_OUTPUT}")
	SET(CROSSCOMPILE "1")
	SET(CROSS "c.")
	MESSAGE(STATUS "Configuring for cross-compiling to ${USED_CC_OUTPUT}")
ELSE (NOT "${HOST_CC_OUTPUT}" STREQUAL "${USED_CC_OUTPUT}")
	SET(CROSSCOMPILE "0")
	SET(CROSS "")
ENDIF (NOT "${HOST_CC_OUTPUT}" STREQUAL "${USED_CC_OUTPUT}")

#################################################################################

# Linux spezifische Parameter
IF ( "${USED_CC_OUTPUT}" MATCHES "linux" )
	
	SET(COMPILEFOR "linux")
	
	IF ( "${USED_CC_OUTPUT}" MATCHES "x86_64" )
		SET(COMPILEARCH "x86_64")
	ELSE ( "${USED_CC_OUTPUT}" MATCHES "x86_64" )
		SET(COMPILEARCH "i386")
	ENDIF ( "${USED_CC_OUTPUT}" MATCHES "x86_64" )
ENDIF ( "${USED_CC_OUTPUT}" MATCHES "linux" )

#################################################################################

# FreeBSD spezifische Parameter
IF ( "${USED_CC_OUTPUT}" MATCHES "freebsd" )

	SET(COMPILEFOR "freebsd")

	IF ( "${USED_CC_OUTPUT}" MATCHES "x86_64" )
		SET(COMPILEARCH "x86_64")
	ELSE ( "${USED_CC_OUTPUT}" MATCHES "x86_64" )
		SET(COMPILEARCH "i386")
	ENDIF ( "${USED_CC_OUTPUT}" MATCHES "x86_64" )
ENDIF ( "${USED_CC_OUTPUT}" MATCHES "freebsd" )

#################################################################################

# Apple spezifische Parameter
IF ( "${USED_CC_OUTPUT}" MATCHES "apple" )
	SET(COMPILEFOR "apple")

	# find lipo
	FIND_PROGRAM(LIPO NAMES ${USED_CC_OUTPUT}-lipo lipo)
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
ENDIF ( "${USED_CC_OUTPUT}" MATCHES "apple" )

#################################################################################

# Cygwin/Mingw spezifische Parameter
IF ( "${USED_CC_OUTPUT}" MATCHES "mingw" OR "${USED_CC_OUTPUT}" MATCHES "cygwin" )
	SET(COMPILEFOR "windows")

    IF ( "${USED_CC_OUTPUT}" MATCHES "x86_64" )
        SET(COMPILEARCH "x86_64")
    ELSE ( "${USED_CC_OUTPUT}" MATCHES "x86_64" )
        SET(COMPILEARCH "i386")
    ENDIF ( "${USED_CC_OUTPUT}" MATCHES "x86_64" )
ENDIF ( "${USED_CC_OUTPUT}" MATCHES "mingw" OR "${USED_CC_OUTPUT}" MATCHES "cygwin" )

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
