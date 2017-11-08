FIND_PACKAGE(Boost 1.55.0 REQUIRED COMPONENTS filesystem system)

file(GLOB ALL_SOURCES *.cpp *.h)

add_library(rttrConfig STATIC ${ALL_SOURCES})
target_include_directories(rttrConfig SYSTEM PUBLIC ${BOOST_INCLUDE_DIR})
target_include_directories(rttrConfig INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(rttrConfig
	PUBLIC s25util
	PUBLIC s25Common
	PUBLIC ${BOOST_LIBRARIES})
SET_PROPERTY(TARGET rttrConfig PROPERTY POSITION_INDEPENDENT_CODE ON)

if(ClangFormat_FOUND)
    add_clangFormat_files(${ALL_SOURCES})
endif()
