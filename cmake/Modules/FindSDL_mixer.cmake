# Copyright (C) 2005 Eric Wing
#
# SPDX-License-Identifier: BSD-3-Clause

# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindSDL_mixer
-------------

Locate SDL_mixer library

This module defines:

::

  SDL_Mixer::SDL_Mixer target to link to
  SDL_MIXER_FOUND, if false, do not try to link against
  SDL_MIXER_VERSION - human-readable string containing the
                             version of SDL_mixer



$SDLDIR is an environment variable that would correspond to the
./configure --prefix=$SDLDIR used in building SDL.

Based on file created by Eric Wing.  This was influenced by the FindSDL.cmake
module, but with modifications to recognize OS X frameworks and
additional Unix paths (FreeBSD, etc).
#]=======================================================================]

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(VC_LIB_PATH_SUFFIX lib/x64)
else()
  set(VC_LIB_PATH_SUFFIX lib/x86)
endif()

if(SDL_mixer_FIND_VERSION VERSION_GREATER_EQUAL 2)
  set(libName SDL2_mixer)
  set(pathSuffixes SDL2 include/SDL2 include)
else()
  set(libName SDL_mixer)
  set(pathSuffixes SDL include/SDL include/SDL12 include/SDL11 include)
endif()

find_library(SDL_MIXER_LIBRARY
  NAMES ${libName}
  HINTS
    ENV SDLMIXERDIR
    ENV SDLDIR
  PATH_SUFFIXES lib ${VC_LIB_PATH_SUFFIX}
)

if(SDL_MIXER_LIBRARY)
  # Use that as a hint to find correct include (not SDL2 header for SDL1)
  if(IS_DIRECTORY "${SDL_MIXER_LIBRARY}")
    set(includeHints ${SDL_MIXER_LIBRARY}/Headers) # Framework path
  else()
    get_filename_component(includeHints "${SDL_MIXER_LIBRARY}" DIRECTORY)
  endif()
  find_path(SDL_MIXER_INCLUDE_DIR SDL_mixer.h
    HINTS ${includeHints}
    NO_DEFAULT_PATH
    PATH_SUFFIXES ${pathSuffixes}
  )
endif()

find_path(SDL_MIXER_INCLUDE_DIR SDL_mixer.h
  HINTS
    ENV SDLMIXERDIR
    ENV SDLDIR
  PATH_SUFFIXES ${pathSuffixes}
)

if(SDL_MIXER_INCLUDE_DIR AND EXISTS "${SDL_MIXER_INCLUDE_DIR}/SDL_mixer.h")
  file(STRINGS "${SDL_MIXER_INCLUDE_DIR}/SDL_mixer.h" SDL_MIXER_VERSION_MAJOR_LINE REGEX "^#define[ \t]+SDL_MIXER_MAJOR_VERSION[ \t]+[0-9]+$")
  file(STRINGS "${SDL_MIXER_INCLUDE_DIR}/SDL_mixer.h" SDL_MIXER_VERSION_MINOR_LINE REGEX "^#define[ \t]+SDL_MIXER_MINOR_VERSION[ \t]+[0-9]+$")
  file(STRINGS "${SDL_MIXER_INCLUDE_DIR}/SDL_mixer.h" SDL_MIXER_VERSION_PATCH_LINE REGEX "^#define[ \t]+SDL_MIXER_PATCHLEVEL[ \t]+[0-9]+$")
  string(REGEX REPLACE "^#define[ \t]+SDL_MIXER_MAJOR_VERSION[ \t]+([0-9]+)$" "\\1" SDL_MIXER_VERSION_MAJOR "${SDL_MIXER_VERSION_MAJOR_LINE}")
  string(REGEX REPLACE "^#define[ \t]+SDL_MIXER_MINOR_VERSION[ \t]+([0-9]+)$" "\\1" SDL_MIXER_VERSION_MINOR "${SDL_MIXER_VERSION_MINOR_LINE}")
  string(REGEX REPLACE "^#define[ \t]+SDL_MIXER_PATCHLEVEL[ \t]+([0-9]+)$" "\\1" SDL_MIXER_VERSION_PATCH "${SDL_MIXER_VERSION_PATCH_LINE}")
  set(SDL_MIXER_VERSION ${SDL_MIXER_VERSION_MAJOR}.${SDL_MIXER_VERSION_MINOR}.${SDL_MIXER_VERSION_PATCH})
  unset(SDL_MIXER_VERSION_MAJOR_LINE)
  unset(SDL_MIXER_VERSION_MINOR_LINE)
  unset(SDL_MIXER_VERSION_PATCH_LINE)
  unset(SDL_MIXER_VERSION_MAJOR)
  unset(SDL_MIXER_VERSION_MINOR)
  unset(SDL_MIXER_VERSION_PATCH)
else()
  set(SDL_MIXER_VERSION "")
endif()

# Find correct SDL dependency
if(SDL_MIXER_VERSION)
  include(CMakeFindDependencyMacro)
  if(SDL_MIXER_VERSION VERSION_LESS "2")
    find_dependency(SDL 1)
    set(additionalVars SDL_FOUND)
  else()
    find_dependency(SDL2 2)
    set(additionalVars SDL2_FOUND)
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SDL_mixer
                                  REQUIRED_VARS SDL_MIXER_LIBRARY SDL_MIXER_INCLUDE_DIR ${additionalVars}
                                  VERSION_VAR SDL_MIXER_VERSION)

if(NOT TARGET SDL_mixer::SDL_mixer)
  add_library(SDL_mixer-SDL_mixer INTERFACE)
  add_library(SDL_mixer::SDL_mixer ALIAS SDL_mixer-SDL_mixer)
  target_link_libraries(SDL_mixer-SDL_mixer INTERFACE ${SDL_MIXER_LIBRARY})
  target_include_directories(SDL_mixer-SDL_mixer SYSTEM INTERFACE ${SDL_MIXER_INCLUDE_DIR})
  # Add correct SDL dependency
  if(SDL_MIXER_VERSION VERSION_LESS "2")
    if(NOT SDL_VERSION_STRING VERSION_LESS "2")
      message(FATAL_ERROR "SDL_Mixer ${SDL_MIXER_VERSION} not compatible with SDL ${SDL_VERSION_STRING}")
    endif()
    target_link_libraries(SDL_mixer-SDL_mixer INTERFACE ${SDL_LIBRARY})
    target_include_directories(SDL_mixer-SDL_mixer SYSTEM INTERFACE ${SDL_INCLUDE_DIR})
  else()
    target_link_libraries(SDL_mixer-SDL_mixer INTERFACE SDL2::SDL2)
  endif()
endif()

mark_as_advanced(SDL_MIXER_LIBRARY SDL_MIXER_INCLUDE_DIR)
