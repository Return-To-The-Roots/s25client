FIND_PACKAGE(Boost 1.55.0 REQUIRED COMPONENTS filesystem system)

set(ALL_SOURCES
    src/files.h
    src/RTTR_Version.cpp
    src/RTTR_Version.h
    src/RttrConfig.cpp
    src/RttrConfig.h)

add_library(rttrConfig STATIC ${ALL_SOURCES})
target_include_directories(rttrConfig SYSTEM PUBLIC ${BOOST_INCLUDE_DIR})
target_include_directories(rttrConfig INTERFACE src)
target_link_libraries(rttrConfig
	PUBLIC s25util
	PUBLIC s25Common
	PUBLIC ${BOOST_LIBRARIES})
SET_PROPERTY(TARGET rttrConfig PROPERTY POSITION_INDEPENDENT_CODE ON)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/build_paths.h.cmake build_paths.h @ONLY)

# Figure out RTTR_REVISION (git hash) and RTTR_VERSION (date)
# Those can be manually set to override the default values
if(NOT RTTR_REVISION)
    include(GetGitRevisionDescription)
    # Get git commit hash. Note: CMake will reconfigure if the git hash changes
    get_git_head_revision(RTTR_GIT_REF RTTR_REVISION)
    if(NOT RTTR_REVISION)
        message(FATAL_ERROR "Could not get git revision. The source has to be in a git repo or you have to define RTTR_REVISION")
    endif()
    set(RTTR_REVISION ${RTTR_REVISION} CACHE STRING "Git revision" FORCE)
endif()
if(NOT RTTR_VERSION)
    STRING(TIMESTAMP RTTR_VERSION "%Y%m%d")
    set(RTTR_VERSION ${RTTR_VERSION} CACHE STRING "Version (yyyymmdd)" FORCE)
endif()
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/build_version_defines.h.cmake build_version_defines.h @ONLY)

target_include_directories(rttrConfig PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

include(EnableWarnings)
enable_warnings(rttrConfig)
