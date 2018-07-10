FIND_PACKAGE(Boost 1.55.0 REQUIRED COMPONENTS filesystem system)

set(ALL_SOURCES files.h
    RTTR_Version.cpp RTTR_Version.h
    RttrConfig.cpp RttrConfig.h)

add_library(rttrConfig STATIC ${ALL_SOURCES})
target_include_directories(rttrConfig SYSTEM PUBLIC ${BOOST_INCLUDE_DIR})
target_include_directories(rttrConfig INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(rttrConfig
	PUBLIC s25util
	PUBLIC s25Common
	PUBLIC ${BOOST_LIBRARIES})
SET_PROPERTY(TARGET rttrConfig PROPERTY POSITION_INDEPENDENT_CODE ON)

CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/build_paths.h.cmake" build_paths.h @ONLY)
ADD_CUSTOM_TARGET(updateversion ALL
				  COMMAND $<TARGET_FILE:version> "${CMAKE_SOURCE_DIR}"
				  DEPENDS version
				  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
                  SOURCES "${CMAKE_SOURCE_DIR}/build_version_defines.h.cmake"
)
ADD_DEPENDENCIES(rttrConfig updateversion) # Create build_version_defines.h
target_include_directories(rttrConfig PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

if(ClangFormat_FOUND)
    add_clangFormat_files(${ALL_SOURCES})
endif()
