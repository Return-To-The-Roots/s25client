# Add an option where the value can only be one of a list of values
# Parameters:
# DEFAULT     -- Default value
# DESCRIPTION -- Description
# VALUES      -- List of allowed values
function(constrained_option varName)
  include(ParseArguments)
  set(options)
  set(one_value_options DEFAULT DESCRIPTION)
  set(multi_value_options VALUES)
  set(required_options DEFAULT DESCRIPTION VALUES)
  parse_arguments("${options}" "${one_value_options}" "${multi_value_options}" "${required_options}" ${ARGN})

  set(${varName} "${ARG_DEFAULT}" CACHE STRING "${ARG_DESCRIPTION}")
  set_property(CACHE ${varName} PROPERTY STRINGS ${ARG_VALUES})
  if(NOT "${${varName}}" IN_LIST ARG_VALUES)
    message(FATAL_ERROR "Value of ${varName} is not any of ${ARG_VALUES}")
  endif()
endfunction()
