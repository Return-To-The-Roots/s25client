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

add_test(NAME MainTest COMMAND Test)
