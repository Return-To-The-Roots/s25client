
MACRO(REMOVE_FLAGS parameter)
	# Make a copy of the current arguments in ${parameter}
	SET(new_parameter " ${${parameter}} ")

	# Now loop over each argument to remove
	FOREACH(argToRemove ${ARGN})
        STRING(REPLACE " ${argToRemove} " " " new_parameter "${new_parameter}")
	ENDFOREACH()
    STRING(REGEX REPLACE "(^ +)|( +$)" "" new_parameter "${new_parameter}")
	SET(${parameter} ${new_parameter} CACHE STRING "" FORCE)
ENDMACRO()
