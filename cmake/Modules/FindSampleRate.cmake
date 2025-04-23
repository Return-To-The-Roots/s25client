# Copyright (C) 2005 - 2025 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#.rst:
# FindSampleRate
# -----------
#
# Find the SampleRate Library (aka Secret Rabbit Code)
#
# Imported Targets
# ^^^^^^^^^^^^^^^^
#
# This module defines the following :prop_tgt:`IMPORTED` targets:
#
# ``SampleRate::samplerate``

find_package(PkgConfig QUIET)

if(PkgConfig_FOUND)
  pkg_check_modules(PC_SampleRate QUIET samplerate)
  if(PC_SampleRate_FOUND)
    find_path(SampleRate_INCLUDE_DIR
      NAMES samplerate.h
      HINTS ${PC_SampleRate_INCLUDE_DIRS}
      NO_DEFAULT_PATH
    )
    find_library(SampleRate_LIBRARY
      NAMES libsamplerate samplerate
      HINTS ${PC_SampleRate_LIBRARY_DIRS} ${PC_SampleRate_STATIC_LIBRARY_DIRS}
      NO_DEFAULT_PATH
    )
  endif()
endif()

find_path(SampleRate_INCLUDE_DIR
  NAMES samplerate.h
  PATH_SUFFIXES libsamplerate SampleRate SampleRate
)

find_library(SampleRate_LIBRARY
    NAMES libsamplerate samplerate
)

if(SampleRate_LIBRARY)
  set(regex "libsamplerate-([0-9]+\\.[0-9]+\\.[0-9]+) ")
  file(STRINGS ${SampleRate_LIBRARY} SampleRate_VERSION_STRING REGEX "${regex}")
  string(REGEX MATCH "${regex}" SampleRate_VERSION_STRING "${SampleRate_VERSION_STRING}")
  if(SampleRate_VERSION_STRING)
    set(SampleRate_VERSION "${CMAKE_MATCH_1}")
  endif()
endif()


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SampleRate
  REQUIRED_VARS SampleRate_LIBRARY SampleRate_INCLUDE_DIR
  VERSION_VAR SampleRate_VERSION
)

if(SampleRate_FOUND)
  if(NOT TARGET SampleRate::samplerate)
    add_library(SampleRate::samplerate UNKNOWN IMPORTED)
    set_target_properties(SampleRate::samplerate PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${SampleRate_INCLUDE_DIR}"
      IMPORTED_LOCATION "${SampleRate_LIBRARY}")
  endif()
endif()

mark_as_advanced(SampleRate_INCLUDE_DIR SampleRate_LIBRARY)
