# - Find SDL2
# Find the SDL2 headers and libraries
#
#  SDL2::SDL2 - Imported target to use for building a library
#  SDL2::SDL2main - Imported interface target to use if you want SDL2main.
#  SDL2_FOUND - True if SDL2 was found.
#  SDL2_DYNAMIC - If we found a DLL version of SDL (meaning you might want to copy a DLL from SDL2::SDL2)
#
# Original Author:
# 2015 Ryan Pavlik <ryan.pavlik@gmail.com> <abiryan@ryand.net>
#
# Copyright Sensics, Inc. 2015.
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(sdl2_extra_required "")

# Invoke pkgconfig for hints
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_search_module(PC_SDL2 QUIET sdl2)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(sdl2_lib_suffix x64)
else()
    set(sdl2_lib_suffix x86)
endif()

find_path(SDL2_INCLUDE_DIR NAMES SDL_haptic.h # this file was introduced with SDL2
    HINTS ${PC_SDL2_INCLUDE_DIRS}
    PATHS ${SDL2_ROOT_DIR}
          ENV SDL2DIR
    PATH_SUFFIXES include/SDL2 SDL2 include
)

find_library(SDL2_LIBRARY NAMES    SDL2
    HINTS ${PC_SDL2_LIBRARY_DIRS} ${PC_SDL2_LIBDIR}
    PATHS ${SDL2_ROOT_DIR}
          ENV SDL2DIR
    PATH_SUFFIXES SDL2 lib ${sdl2_lib_suffix} lib/${sdl2_lib_suffix}
)

if(WIN32 AND SDL2_LIBRARY)
    find_file(SDL2_RUNTIME_LIBRARY NAMES SDL2.dll libSDL2.dll
        HINTS ${PC_SDL2_LIBRARY_DIRS} ${PC_SDL2_LIBDIR}
        PATHS ${SDL2_ROOT_DIR}
              ENV SDL2DIR
        PATH_SUFFIXES bin lib ${sdl2_lib_suffix} bin/${sdl2_lib_suffix}
    )
endif()

if(NOT SDL2_INCLUDE_DIR MATCHES ".framework")
    list(APPEND sdl2_extra_required SDL2_SDL2MAIN_LIBRARY)
    find_library(SDL2_SDL2MAIN_LIBRARY NAMES SDL2main
        HINTS ${PC_SDL2_LIBRARY_DIRS} ${PC_SDL2_LIBDIR}
        PATHS ${SDL2_ROOT_DIR}
              ENV SDL2DIR
        PATH_SUFFIXES SDL2 lib ${sdl2_lib_suffix} lib/${sdl2_lib_suffix}
    )
endif()

if(MINGW)
    find_library(SDL2_MINGW_LIBRARY mingw32)
    list(APPEND sdl2_extra_required SDL2_MINGW_LIBRARY)
endif()

if(SDL2_INCLUDE_DIR AND EXISTS "${SDL2_INCLUDE_DIR}/SDL_version.h")
    file(STRINGS "${SDL2_INCLUDE_DIR}/SDL_version.h" SDL2_VERSION_MAJOR_LINE REGEX "^#define[ \t]+SDL_MAJOR_VERSION[ \t]+[0-9]+$")
    file(STRINGS "${SDL2_INCLUDE_DIR}/SDL_version.h" SDL2_VERSION_MINOR_LINE REGEX "^#define[ \t]+SDL_MINOR_VERSION[ \t]+[0-9]+$")
    file(STRINGS "${SDL2_INCLUDE_DIR}/SDL_version.h" SDL2_VERSION_PATCH_LINE REGEX "^#define[ \t]+SDL_PATCHLEVEL[ \t]+[0-9]+$")
    string(REGEX REPLACE "^#define[ \t]+SDL_MAJOR_VERSION[ \t]+([0-9]+)$" "\\1" SDL2_VERSION_MAJOR "${SDL2_VERSION_MAJOR_LINE}")
    string(REGEX REPLACE "^#define[ \t]+SDL_MINOR_VERSION[ \t]+([0-9]+)$" "\\1" SDL2_VERSION_MINOR "${SDL2_VERSION_MINOR_LINE}")
    string(REGEX REPLACE "^#define[ \t]+SDL_PATCHLEVEL[ \t]+([0-9]+)$" "\\1" SDL2_VERSION_PATCH "${SDL2_VERSION_PATCH_LINE}")
    set(SDL2_VERSION ${SDL2_VERSION_MAJOR}.${SDL2_VERSION_MINOR}.${SDL2_VERSION_PATCH})
    unset(SDL2_VERSION_MAJOR_LINE)
    unset(SDL2_VERSION_MINOR_LINE)
    unset(SDL2_VERSION_PATCH_LINE)
    unset(SDL2_VERSION_MAJOR)
    unset(SDL2_VERSION_MINOR)
    unset(SDL2_VERSION_PATCH)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SDL2
    REQUIRED_VARS SDL2_INCLUDE_DIR SDL2_LIBRARY ${sdl2_extra_required}
    VERSION_VAR SDL2_VERSION
)

if(SDL2_FOUND)
    if(NOT TARGET SDL2::SDL2)
        if(WIN32 AND SDL2_RUNTIME_LIBRARY)
            set(SDL2_DYNAMIC TRUE)
            add_library(SDL2::SDL2 SHARED IMPORTED)
            set_target_properties(SDL2::SDL2 PROPERTIES
                IMPORTED_IMPLIB "${SDL2_LIBRARY}"
                IMPORTED_LOCATION "${SDL2_RUNTIME_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIR}"
            )
        else()
            add_library(SDL2::SDL2 UNKNOWN IMPORTED)
        if(APPLE AND SDL2_LIBRARY MATCHES "/([^/]*).framework$")
            set(sdl2_location "${SDL2_LIBRARY}/${CMAKE_MATCH_1}")
        else()
        set(sdl2_location "${SDL2_LIBRARY}")
        endif()
            set_target_properties(SDL2::SDL2 PROPERTIES
                IMPORTED_LOCATION "${sdl2_location}"
                INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIR}"
            )
        endif()

        if(APPLE)
            # Need Cocoa here
            set_target_properties(SDL2::SDL2 PROPERTIES
                IMPORTED_LINK_INTERFACE_LIBRARIES "-framework Cocoa"
            )
        endif()
    endif()

    # Compute what to do with SDL2main
    if(NOT TARGET SDL2::SDL2main)
        if(SDL2_SDL2MAIN_LIBRARY)
            add_library(SDL2::SDL2main STATIC IMPORTED)
            set_target_properties(SDL2::SDL2main PROPERTIES
                IMPORTED_LOCATION "${SDL2_SDL2MAIN_LIBRARY}"
                INTERFACE_COMPILE_DEFINITIONS "main=SDL_main"
            )
            if(MINGW AND SDL2_MINGW_LIBRARY)
                set_target_properties(SDL2::SDL2main PROPERTIES
                    INTERFACE_LINK_LIBRARIES "${SDL2_MINGW_LIBRARY} -mwindows"
                )
            endif()
        else()
            # Dummy target
            add_library(SDL2::SDL2main INTERFACE IMPORTED)
        endif()
    endif()
endif()

mark_as_advanced(SDL2_LIBRARY
    SDL2_RUNTIME_LIBRARY
    SDL2_INCLUDE_DIR
    SDL2_SDL2MAIN_LIBRARY
    SDL2_MINGW_LIBRARY
)
