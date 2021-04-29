# Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# Meta target that depends on all drivers
add_custom_target(drivers)

# Minimal visibility to avoid name clashes among plugins
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

add_subdirectory(audioDrivers)
add_subdirectory(videoDrivers)
if(RTTR_BUNDLE AND APPLE)
    add_subdirectory(macosLauncher)
endif()

if(MSVC)
  option(RTTR_ENABLE_VLD "Enable use of Visual Leak Detector" OFF)
  option(RTTR_ENABLE_VLD_FORCED "Force using of Visual Leak Detector (in non-Debug builds)" OFF)
else()
  set(RTTR_ENABLE_VLD OFF)
endif()
add_library(rttr_vld INTERFACE)
add_library(rttr::vld ALIAS rttr_vld)
target_compile_definitions(rttr_vld INTERFACE RTTR_HAS_VLD=$<BOOL:${RTTR_ENABLE_VLD}>)
if(RTTR_ENABLE_VLD)
  find_package(VLD 2.5.1 REQUIRED)
  target_link_libraries(rttr_vld INTERFACE vld::vld)
  if(RTTR_ENABLE_VLD_FORCED)
    target_compile_definitions(rttr_vld INTERFACE VLD_FORCE_ENABLE)
  endif()
endif()
