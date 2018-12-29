# Add a new test case for boost tests with working directory in the binary dir root
# Params:
# NAME <name>
# SOURCES/LIBS/INCLUDES <value, value, ...>
function(add_testcase)
    set(options )
    set(oneValueArgs NAME)
    set(multiValueArgs LIBS INCLUDES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    string(MAKE_C_IDENTIFIER "Test_${ARG_NAME}" name)
    file(GLOB sources *.[ch]pp *.[hc] *.cxx)
    add_executable(${name} ${sources})
    target_link_libraries(${name} PRIVATE ${ARG_LIBS}
		PRIVATE ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
	)
    target_include_directories(${name} PRIVATE ${ARG_INCLUDES})
    if(NOT Boost_USE_STATIC_LIBS)
        target_compile_definitions(${name} PRIVATE BOOST_TEST_DYN_LINK)
    endif()
    if(MSVC)
        # Use full paths for defines (makes Boost.Test with the VS addin work better)
        target_compile_options(${name} PUBLIC /FC)
    endif()
    add_test(NAME ${name} COMMAND ${name} WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
endfunction()
