# Copyright (C) 2018 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# Library interface to be consumed by users
add_library(driver INTERFACE)
target_include_directories(driver INTERFACE include)
target_link_libraries(driver INTERFACE Boost::boost)

include(EnableWarnings)
enable_warnings(driver)

# Base libraries to be used by driver implementations
set(RTTR_DRIVER_INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src/DriverInterface.cpp CACHE INTERNAL "")

add_library(videodrv STATIC
    src/VideoDriver.cpp
    include/driver/DriverInterfaceVersion.h
    include/driver/exportImport.h
    include/driver/Interface.h
    include/driver/KeyEvent.h
    include/driver/MouseCoords.h
    include/driver/VideoDriver.h
    include/driver/VideoDriverLoaderInterface.h
    include/driver/VideoInterface.h
    include/driver/VideoMode.h
)
target_link_libraries(videodrv PUBLIC driver s25Common)
target_compile_definitions(videodrv PUBLIC BUILD_DLL)
enable_warnings(videodrv)

add_library(audiodrv STATIC
    src/AudioDriver.cpp
    include/driver/DriverInterfaceVersion.h
    include/driver/EffectPlayId.h
    include/driver/exportImport.h
    include/driver/IAudioDriverCallback.h
    include/driver/Interface.h
    include/driver/AudioDriver.h
    include/driver/AudioInterface.h
    include/driver/RawSoundHandle.h
)
target_link_libraries(audiodrv PUBLIC driver s25Common)
target_compile_definitions(audiodrv PUBLIC BUILD_DLL)
enable_warnings(audiodrv)

set_target_properties(videodrv audiodrv PROPERTIES POSITION_INDEPENDENT_CODE ON)
