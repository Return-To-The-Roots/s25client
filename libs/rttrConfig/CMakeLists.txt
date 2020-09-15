add_library(rttrConfig STATIC
  src/files.h
  src/RTTR_Version.cpp
  src/RTTR_Version.h
  src/RttrConfig.cpp
  src/RttrConfig.h
)  
target_include_directories(rttrConfig INTERFACE src)
target_link_libraries(rttrConfig
	PUBLIC s25util::common Boost::filesystem
	PRIVATE s25Common
)
set_property(TARGET rttrConfig PROPERTY POSITION_INDEPENDENT_CODE ON)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/build_paths.h.cmake build_paths.h @ONLY)

# This should happen in the top-level CMakeLists
if(NOT RTTR_REVISION OR NOT RTTR_VERSION)
    message(FATAL_ERROR "Internal error: RTTR_REVISION or RTTR_VERSION not set")
endif()
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/build_version_defines.h.cmake build_version_defines.h @ONLY)

target_include_directories(rttrConfig PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

include(EnableWarnings)
enable_warnings(rttrConfig)
