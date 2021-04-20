# Copyright (C) 2018 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#.rst:
# FindSamplerate
# -----------
#
# Find the Samplerate Library (aka Secret Rabbit Code)
#
# Imported Targets
# ^^^^^^^^^^^^^^^^
#
# This module defines the following :prop_tgt:`IMPORTED` targets:
#
# ``Samplerate::Samplerate``

find_package(PkgConfig QUIET)

if(PkgConfig_FOUND)
  pkg_check_modules(PC_Samplerate QUIET samplerate)
  if(PC_Samplerate_FOUND)
    find_path(Samplerate_INCLUDE_DIR
      NAMES samplerate.h
      HINTS ${PC_Samplerate_INCLUDE_DIRS}
      NO_DEFAULT_PATH
    )
    find_library(Samplerate_LIBRARY
      NAMES libsamplerate samplerate
      HINTS ${PC_Samplerate_LIBRARY_DIRS} ${PC_Samplerate_STATIC_LIBRARY_DIRS}
      NO_DEFAULT_PATH
    )
  endif()
endif()

find_path(Samplerate_INCLUDE_DIR
  NAMES samplerate.h
  PATH_SUFFIXES libsamplerate Samplerate
)

find_library(Samplerate_LIBRARY
    NAMES libsamplerate samplerate
)

if(Samplerate_LIBRARY)
  set(regex "libsamplerate-([0-9]+\\.[0-9]+\\.[0-9]+) ")
  file(STRINGS ${Samplerate_LIBRARY} Samplerate_VERSION_STRING REGEX "${regex}")
  string(REGEX MATCH "${regex}" Samplerate_VERSION_STRING "${Samplerate_VERSION_STRING}")
  if(Samplerate_VERSION_STRING)
    set(Samplerate_VERSION "${CMAKE_MATCH_1}")
  endif()
endif()


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Samplerate
  REQUIRED_VARS Samplerate_LIBRARY Samplerate_INCLUDE_DIR
  VERSION_VAR Samplerate_VERSION
)

if(Samplerate_FOUND)
  if(NOT TARGET Samplerate::Samplerate)
    add_library(Samplerate::Samplerate UNKNOWN IMPORTED)
    set_target_properties(Samplerate::Samplerate PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${Samplerate_INCLUDE_DIR}"
      IMPORTED_LOCATION "${Samplerate_LIBRARY}")
  endif()
endif()

mark_as_advanced(Samplerate_INCLUDE_DIR Samplerate_LIBRARY)
