

add_subdirectory(mockupDrivers)

find_package(Boost REQUIRED COMPONENTS unit_test_framework)
if(NOT Boost_USE_STATIC_LIBS)
	ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK) 
endif()
if(MSVC)
    # Use full paths for defines (makes Boost.Test with the VS addin work better)
    add_definitions(/FC)
endif()

get_filename_component(RTTR_BASE_DIR "${CMAKE_SOURCE_DIR}" ABSOLUTE)
set(RTTR_LIBSIEDLER2_TEST_FILES_DIR "${RTTR_BASE_DIR}/libsiedler2/examples/test/testFiles")
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/testConfig.h.cmake" include/test/testConfig.h @ONLY)
unset(RTTR_BASE_DIR)
unset(RTTR_LIBSIEDLER2_TEST_FILES_DIR)

file(GLOB OTHER_SRC *.cpp *.h)
FILE(GLOB LEGACY_FILES legacy/*.cpp legacy/*.h)
SET(TEST_SOURCES ${OTHER_SRC} ${LEGACY_FILES})
SOURCE_GROUP(legacy FILES ${LEGACY_FILES})

add_executable(Test ${TEST_SOURCES})
target_link_libraries(Test
						s25Main
                        audioMockup
                        videoMockup
						${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
					  )
target_include_directories(Test PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/include")
add_test(NAME MainTest COMMAND Test WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")

file(GLOB TEST_CASES test*.cpp)
source_group(testCases FILES ${TEST_CASES})
foreach(curFile ${TEST_CASES})
    list(REMOVE_ITEM OTHER_SRC ${curFile})
endforeach()
source_group(other FILES ${OTHER_SRC})

if(WIN32)
    include(GatherDll)
    gather_dll_copy(Test)
endif()
