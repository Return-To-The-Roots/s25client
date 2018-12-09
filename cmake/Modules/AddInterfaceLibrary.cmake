set(__interface_library_dummy_file ${CMAKE_CURRENT_LIST_DIR}/dummy.cpp CACHE INTERNAL "")
# Compatibility
function(add_interface_library name)
	if(CMAKE_VERSION VERSION_LESS 3.0)
			add_library(${name} STATIC ${__interface_library_dummy_file})
	else()
			add_library(${name} INTERFACE)
	endif()
endfunction()
