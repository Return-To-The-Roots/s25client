# Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

add_library(videoMockup STATIC MockupVideoDriver.h MockupVideoDriver.cpp)
target_link_libraries(videoMockup PUBLIC videodrv s25util::common PRIVATE Boost::nowide)
target_include_directories(videoMockup INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
enable_warnings(videoMockup)

add_library(audioMockup STATIC MockupAudioDriver.h MockupAudioDriver.cpp)
target_link_libraries(audioMockup PUBLIC audiodrv turtle::turtle)
target_include_directories(audioMockup INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
enable_warnings(audioMockup)
