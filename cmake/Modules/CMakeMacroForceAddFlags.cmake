################################################################################
### $Id: CMakeMacroForceAddFlags.cmake 6807 2010-10-18 14:12:04Z FloSoft $
################################################################################

MACRO(FORCE_ADD_FLAGS parameter)
	# Create a separated list of the arguments to loop over
	SET(p_list ${${parameter}})
	SEPARATE_ARGUMENTS(p_list)
	
	# Make a copy of the current arguments in ${parameter}
	SET(new_parameter ${${parameter}})

	# fix arch-argument bug	
	STRING(REPLACE "-arch;" "-arch=" new_parameter "${new_parameter}")

	# Now loop over each required argument and see if it is in our
	# current list of arguments.
	FOREACH(required_arg ${ARGN})
		# This helps when we get arguments to the function that are
		# grouped as a string:
		#
		# ["-O3 -g"]  instead of [-O3 -g]
		SET(TMP ${required_arg}) #elsewise the Seperate command doesn't work)
		SEPARATE_ARGUMENTS(TMP)
		
		FOREACH(option ${TMP})
			# Look for the required argument in our list of existing arguments
			SET(found FALSE)
			
			FOREACH(p_arg ${p_list})
				IF (${p_arg} STREQUAL ${option})
					SET(found TRUE)
				ENDIF (${p_arg} STREQUAL ${option})
			ENDFOREACH(p_arg)
			
			IF(NOT found)
				# The required argument wasn't found, so we need to add it in.
				SET(new_parameter "${new_parameter} ${option}")
			ENDIF(NOT found)
		ENDFOREACH(option ${TMP})
	ENDFOREACH(required_arg ${ARGN})
	
        # unfix arch-argument bug
        STRING(REPLACE "-arch=" "-arch " new_parameter "${new_parameter}")

	SET(${parameter} ${new_parameter} CACHE STRING "" FORCE)
ENDMACRO(FORCE_ADD_FLAGS)
