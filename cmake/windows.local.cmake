################################################################################
### $Id: windows.local.cmake 6807 2010-10-18 14:12:04Z FloSoft $
################################################################################

# this one is important
EXECUTE_PROCESS(COMMAND "uname"
	OUTPUT_VARIABLE CMAKE_SYSTEM_NAME OUTPUT_STRIP_TRAILING_WHITESPACE)
EXECUTE_PROCESS(COMMAND "uname" "-m"
	OUTPUT_VARIABLE CMAKE_SYSTEM_PROCESSOR OUTPUT_STRIP_TRAILING_WHITESPACE)

# specify the compiler
SET_CCACHE(CMAKE_C_COMPILER   gcc)
SET_CCACHE(CMAKE_CXX_COMPILER g++)

INCLUDE(cmake/windows.common.cmake)
