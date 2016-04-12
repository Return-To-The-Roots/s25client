# VS does not support coverage analysis
if(NOT MSVC)
	set(RTTR_ENABLE_COVERAGE OFF CACHE BOOL "Generate coverage build")
	if(RTTR_ENABLE_COVERAGE)
		if(NOT ${CMAKE_BUILD_TYPE} EQUAL Debug)
			message(FATAL "Coverage requires a debug build or you get false positives")
		endif()
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -coverage")
	endif()
endif()

find_package(Boost COMPONENTS unit_test_framework REQUIRED)
if(NOT Boost_USE_STATIC_LIBS)
	ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK) 
endif()
FILE(GLOB TEST_SOURCES *.cpp *.h)
add_executable(Test ${TEST_SOURCES})
target_link_libraries(Test
						s25Main
						${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
					  )

add_test(NAME MainTest COMMAND Test WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

if(MSVC)
	create_target_launcher(Test WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
endif()
