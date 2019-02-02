find_package(Boost 1.64.0 REQUIRED)

# Library interface to be consumed by users
add_library(driver INTERFACE)
target_include_directories(driver INTERFACE include)
target_include_directories(driver SYSTEM INTERFACE Boost::boost)

include(EnableWarnings)
enable_warnings(driver)

# Base libraries to be used by driver implementations
set(RTTR_DRIVER_INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src/DriverInterface.cpp CACHE INTERNAL "")

add_library(videodrv STATIC
    src/VideoDriver.cpp
    include/driver/VideoDriver.h
    include/driver/VideoInterface.h)
target_link_libraries(videodrv PUBLIC driver s25Common)
target_compile_definitions(videodrv PUBLIC BUILD_DLL)

add_library(audiodrv STATIC
    src/AudioDriver.cpp
    include/driver/AudioDriver.h
    include/driver/AudioInterface.h
    include/driver/SoundHandle.h)
target_link_libraries(audiodrv PUBLIC driver s25Common)
target_compile_definitions(audiodrv PUBLIC BUILD_DLL)

set_target_properties(videodrv audiodrv PROPERTIES POSITION_INDEPENDENT_CODE ON)
