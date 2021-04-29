# Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later
#.rst:
# FindVld
# -----------
#
# Find Visual Leak Detector
#
# Imported Targets
# ^^^^^^^^^^^^^^^^
#
# This module defines the following :prop_tgt:`IMPORTED` targets:
#
# ``vld::vld``


find_path(VLD_ROOT_DIR
  NAMES include/vld.h
  PATHS ENV VLDROOT
              "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Visual Leak Detector;InstallPath]"
  PATH_SUFFIXES "Visual Leak Detector")

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
  set(_libSuffix lib/Win32)
else()
  set(_libSuffix lib/Win64)
endif()

find_path(VLD_LIBRARY_DIR
  NAMES vld.lib
  PATHS ${VLD_ROOT_DIR}
  PATH_SUFFIXES ${_libSuffix}
  NO_DEFAULT_PATH)

if(VLD_ROOT_DIR)
  set(_versionFile ${VLD_ROOT_DIR}/CHANGES.txt)

  if(EXISTS ${_versionFile})
    # Examples:
    # Visual Leak Detector (VLD) Version 2.5.1
    # Visual Leak Detector (VLD) Version 2.4RC2
    # Visual Leak Detector (VLD) Version 2.0h
    set(_versionRegex "Visual Leak Detector \\(VLD\\) Version ([0-9]+\\.[0-9]+(.[0-9]+)?)")
    file(STRINGS ${_versionFile} _versionString REGEX ${_versionRegex})
    string(REGEX MATCH ${_versionRegex} _versionString "${_versionString}") # Remove everything but the match
    string(REGEX REPLACE ${_versionRegex} "\\1" VLD_VERSION "${_versionString}") # Keep only the version
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(VLD
  REQUIRED_VARS VLD_ROOT_DIR VLD_LIBRARY_DIR
  VERSION_VAR VLD_VERSION
)

if(VLD_FOUND)
  add_library(vld::vld INTERFACE IMPORTED)
  set_target_properties(vld::vld PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${VLD_ROOT_DIR}/include)
endif()
