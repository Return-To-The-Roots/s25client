get_filename_component(RTTR_BASE_DIR "${CMAKE_SOURCE_DIR}" ABSOLUTE)
set(RTTR_LIBSIEDLER2_TEST_FILES_DIR "${RTTR_BASE_DIR}/external/libsiedler2/tests/testFiles")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/testConfig.h.cmake include/test/testConfig.h @ONLY)
unset(RTTR_BASE_DIR)
unset(RTTR_LIBSIEDLER2_TEST_FILES_DIR)

add_library(testConfig INTERFACE)
target_include_directories(testConfig INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/include)

add_library(testWorldFixtures STATIC
    worldFixtures/CreateEmptyWorld.cpp
    worldFixtures/CreateEmptyWorld.h
    worldFixtures/CreateSeaWorld.cpp
    worldFixtures/CreateSeaWorld.h
    worldFixtures/GCExecutor.h
    worldFixtures/initGameRNG.cpp
    worldFixtures/initGameRNG.hpp
    worldFixtures/SeaWorldWithGCExecution.h
    worldFixtures/TestEventManager.cpp
    worldFixtures/TestEventManager.h
    worldFixtures/WorldFixture.h
    worldFixtures/WorldWithGCExecution.h
)
target_link_libraries(testWorldFixtures PUBLIC s25Main PRIVATE testHelpers)
target_include_directories(testWorldFixtures PUBLIC .)
enable_warnings(testWorldFixtures)

add_subdirectory(audio)
# Don't include this test when collecting coverage data
if(NOT RTTR_ENABLE_COVERAGE)
    add_subdirectory(autoplay)
endif()
add_subdirectory(drivers)
add_subdirectory(integration)
add_subdirectory(IO)
add_subdirectory(locale)
add_subdirectory(lua)
add_subdirectory(mapGenerator)
add_subdirectory(network)
add_subdirectory(notificationSystem)
add_subdirectory(resources)
add_subdirectory(simple)
add_subdirectory(UI)

option(RTTR_ENABLE_BENCHMARKS "Build the benchmarks" OFF)
if(RTTR_ENABLE_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()

if(WIN32)
    include(GatherDll)
    gather_dll_copy(testWorldFixtures)
endif()
