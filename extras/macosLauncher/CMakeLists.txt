# Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

if(NOT RTTR_BUNDLE OR NOT RTTR_MACOS_DIR)
    message(FATAL_ERROR "Can only use this in a bundle")
endif()

add_executable(starter starter.cpp)
target_link_libraries(starter PRIVATE s25util::common rttrConfig Boost::nowide)
set(MACOSX_BUNDLE_EXECUTABLE_NAME "starter${CMAKE_EXECUTABLE_SUFFIX}")
set(MACOSX_BUNDLE_BUNDLE_VERSION "${RTTR_VERSION}")
set(MACOSX_BUNDLE_COPYRIGHT "(C) 2005-2018 Settlers Freaks")
set(MACOSX_BUNDLE_LONG_VERSION_STRING "${RTTR_REVISION}")
set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${RTTR_VERSION}")
configure_file(Info.plist.in ${PROJECT_BINARY_DIR}/Info.plist @ONLY)
install(TARGETS starter DESTINATION ${RTTR_BINDIR})
install(FILES rttr.terminal DESTINATION ${RTTR_BINDIR})
install(FILES ${PROJECT_BINARY_DIR}/Info.plist DESTINATION ${RTTR_MACOS_DIR}/..)
install(FILES icon.icns DESTINATION ${RTTR_MACOS_DIR}/../Resources)
