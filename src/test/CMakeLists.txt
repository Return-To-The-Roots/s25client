find_package(Boost COMPONENTS unit_test_framework REQUIRED)
if(NOT Boost_USE_STATIC_LIBS)
	ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK) 
endif()
if(MSVC)
    # Use full paths for defines (makes Boost.Test with the VS addin work better)
    add_definitions(/FC)
endif()
file(GLOB TEST_SOURCES *.cpp *.h)
add_executable(Test ${TEST_SOURCES} ${CMAKE_SOURCE_DIR}/src/ProgramInitHelpers.cpp ${CMAKE_SOURCE_DIR}/driver/src/AudioDriver.cpp ${CMAKE_SOURCE_DIR}/driver/src/VideoDriver.cpp)
target_link_libraries(Test
						s25Main
						${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
					  )

add_test(NAME MainTest COMMAND Test WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

file(GLOB TEST_CASES test*.cpp)
source_group(testCases FILES ${TEST_CASES})
set(OTHER_SRC ${TEST_SOURCES})
foreach(curFile ${TEST_CASES})
    list(REMOVE_ITEM OTHER_SRC ${curFile})
endforeach()
source_group(other FILES ${OTHER_SRC})

if(MSVC)
	create_target_launcher(Test WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
else()
	SET_TARGET_PROPERTIES(Test PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/${RTTR_BINDIR}"
		RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/${RTTR_BINDIR}"
		RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/${RTTR_BINDIR}"
	)
endif()
