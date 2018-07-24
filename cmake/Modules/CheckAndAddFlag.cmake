INCLUDE(CheckCXXCompilerFlag)
INCLUDE(CheckCCompilerFlag)

function(CheckAndAddFlag flag)
	# We cannot check for -Wno-foo as this won't throw a warning so we must check for the -Wfoo option directly
	# http://stackoverflow.com/questions/38785168/cc1plus-unrecognized-command-line-option-warning-on-any-other-warning
	STRING(REGEX REPLACE "^-Wno-" "-W" checkedFlag ${flag})
    # Remove special chars
    string(MAKE_C_IDENTIFIER ${checkedFlag} VarName)
	CHECK_CXX_COMPILER_FLAG(${checkedFlag} CXX_FLAG_${VarName}_SUPPORTED)
	CHECK_C_COMPILER_FLAG(${checkedFlag} C_FLAG_${VarName}_SUPPORTED)
	IF(CXX_FLAG_${VarName}_SUPPORTED)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}" PARENT_SCOPE)
	ENDIF()
	IF(C_FLAG_${VarName}_SUPPORTED)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${flag}" PARENT_SCOPE)
	ENDIF()
endfunction()

function(CheckAndAddFlags)
    foreach(flag ${ARGN})
        CheckAndAddFlag(${flag})
    endforeach()
endfunction()