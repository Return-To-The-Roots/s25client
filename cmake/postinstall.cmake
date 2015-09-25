
MESSAGE(STATUS "Running postinstall script")

# run install script
EXECUTE_PROCESS(
	COMMAND bash "postinstall.sh"
	RESULT_VARIABLE POSTINSTALL_RESULT
)

IF(NOT "${POSTINSTALL_RESULT}" STREQUAL "0")
	MESSAGE(FATAL_ERROR "ERROR: Postinstallscript failed: Maybe you need administrative privileges?")
ENDIF(NOT "${POSTINSTALL_RESULT}" STREQUAL "0")

MESSAGE(STATUS "Done")

###############################################################################
