# Copyright (C) 2005 - 2025 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

cmake_minimum_required(VERSION 3.16..3.31)
if(POLICY CMP0177)
  cmake_policy(SET CMP0177 NEW)  # Normalize install() DESTINATION paths
endif()
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 OLD)  # FindBoost is removed, keep for now
endif()

if(NOT RTTR_VERSION)
    string(TIMESTAMP RTTR_VERSION "%Y%m%d")
endif()

project(RTTR VERSION ${RTTR_VERSION})

message(STATUS "Using CMake ${CMAKE_VERSION}")

if(DEFINED CMAKE_TOOLCHAIN_FILE)
    message(STATUS "Used Toolchain definition file '${CMAKE_TOOLCHAIN_FILE}'")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules" "${CMAKE_SOURCE_DIR}/external/libutil/cmake")

include(EnableCCache)

set(checkSubmodules FALSE)
# Figure out RTTR_REVISION (git hash) and RTTR_VERSION (date)
# Those can be manually set to override the default values
# By default those are non-Cache variables which regenerates them on every configure run
if(NOT RTTR_REVISION)
    include(GetGitRevisionDescription)
    # Get git commit hash. Note: CMake will reconfigure if the git hash changes
    get_git_head_revision(RTTR_GIT_REF RTTR_REVISION)
    if(RTTR_REVISION)
        string(LENGTH "${RTTR_REVISION}" revisionLen)
        if(NOT revisionLen EQUAL "40")
            message(FATAL_ERROR "get_git_head_revision returned a wrong revision: '${RTTR_REVISION}'")
        endif()
        set(checkSubmodules TRUE)
    else()
        set(revisionFile "${CMAKE_CURRENT_LIST_DIR}/revision.txt")
        if(EXISTS ${revisionFile})
            file(READ ${revisionFile} revision)
            string(STRIP "${revision}" revision)
            string(LENGTH "${revision}" revisionLen)
            if(NOT revision MATCHES "^[0-9a-f]+$" OR NOT revisionLen EQUAL "40")
                message(FATAL_ERROR "Contents of ${revisionFile} do not contain a valid revision: '${RTTR_REVISION}'")
            endif()
            set(RTTR_REVISION ${revision})
            # Reconfigure if the file changes
            configure_file(${revisionFile} revision.marker COPYONLY)
        else()
            message(FATAL_ERROR "Could not get git revision. The source has to be in a git repo or you have to define RTTR_REVISION")
        endif()
    endif()
endif()


# stable build? then set rttr.bat/rttr.sh parameter
set(STABLE_PARAM "")
if(RTTR_VERSION MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)$")
    message(STATUS "Creating a stable version ${RTTR_VERSION}")
    set(STABLE_PARAM "--stable")
endif()

if(checkSubmodules)
    include(CheckGitSubmodules)
    check_git_submodules()
endif()

# Just include required
include(EnsureOutOfSourceBuild)

if(APPLE)
    set(CMAKE_INSTALL_RPATH "@executable_path" "@executable_path/../Frameworks")
elseif(NOT WIN32)
    set(CMAKE_INSTALL_RPATH "\$ORIGIN" "\$ORIGIN/../lib")
endif()

################################################################################
# Include platform specific config files
################################################################################

string(TOLOWER ${CMAKE_SYSTEM_NAME} LOWER_SYSTEM_NAME)
if(CMAKE_CROSSCOMPILING)
    message(STATUS "Configuring for cross-compiling to ${CMAKE_SYSTEM} on ${CMAKE_SYSTEM_PROCESSOR}")
    #Include cross-compile platform config
    set(_config "cmake/c.${LOWER_SYSTEM_NAME}.cmake")
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/${_config}")
        message(STATUS "Using cross-platform config ${_config}")
        include("${_config}")
    endif()
else()
    message(STATUS "Configuring for native compiling to ${CMAKE_SYSTEM} on ${CMAKE_SYSTEM_PROCESSOR}")
endif()

#Include generic platform config
set(_config "cmake/${LOWER_SYSTEM_NAME}.cmake")
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/${_config}")
    message(STATUS "Using platform config ${_config}")
    include("${_config}")
endif()

option(RTTR_ENABLE_OPTIMIZATIONS "Build with optimizing flags (such as -O2 and -ffast-math added to CFLAGS and CXXFLAGS)" ON)
if(RTTR_ENABLE_OPTIMIZATIONS)
    include("cmake/optimizations.cmake")
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -ggdb")
endif()

# Better portability
set(CMAKE_CXX_EXTENSIONS OFF)

# Hidden visibility by default to reduce binary size and possibly increase performance
set(CMAKE_CXX_VISIBILITY_PRESET hidden CACHE STRING "Default symbol visibility")
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON CACHE BOOL "Inline symbol visibility")

################################################################################
# ClangFormat
################################################################################

find_package(ClangFormat 10 EXACT)

################################################################################
# Set paths.
# Note: Mixing relative and absolute paths is not recommended!
# If RTTR_BINDIR is absolute then relative paths are relative to CMAKE_INSTALL_PREFIX
# Else a prefix path is calculated based on RTTR_BINDIR and the current executable path so that we have <PrefixPath>/RTTR_BINDIR
# In this case all relative paths are relative to that prefix path
# Example: RTTR_BINDIR="bin", RTTR_DATADIR="data" executed in "/foo/bar/bin/rttr.exe" -> FULL_DATADIR=/foo/bar/data
################################################################################

option(RTTR_BUNDLE "Create a bundle directory (contains all required dependencies)" ${APPLE})
if(WIN32)
    # Windows uses one directory only
    set(RTTR_BINDIR "." CACHE INTERNAL "")
    set(RTTR_DATADIR "." CACHE INTERNAL "")
    set(RTTR_GAMEDIR "." CACHE INTERNAL "")
    set(RTTR_LIBDIR "." CACHE INTERNAL "")
    set(RTTR_DOCDIR "." CACHE INTERNAL "")
    set(RTTR_EXTRA_BINDIR "./RTTR" CACHE INTERNAL "")
    set(RTTR_DRIVERDIR "./driver" CACHE INTERNAL "")
else()
    if(APPLE AND RTTR_BUNDLE)
        set(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app)
        set(RTTR_MACOS_DIR Contents/MacOS CACHE INTERNAL "")
        set(RTTR_DATADIR ${RTTR_MACOS_DIR}/share/s25rttr CACHE INTERNAL "")
        set(RTTR_LIBDIR ${RTTR_MACOS_DIR} CACHE INTERNAL "")
        set(RTTR_DOCDIR ${RTTR_MACOS_DIR}/share/doc CACHE INTERNAL "")
        # (At least) bundling assumes, that all executables are in same folder inside MacOS
        # Additionally main executable is assumed to be in MacOS
        set(RTTR_BINDIR ${RTTR_MACOS_DIR} CACHE INTERNAL "")
        set(RTTR_EXTRA_BINDIR ${RTTR_BINDIR} CACHE INTERNAL "")
        set(RTTR_GAMEDIR ${RTTR_DATADIR}/S2 CACHE INTERNAL "")
        set(RTTR_DRIVERDIR ${RTTR_LIBDIR}/driver CACHE INTERNAL "")
    else()
        include(GNUInstallDirs)
        set(RTTR_BINDIR "${CMAKE_INSTALL_BINDIR}" CACHE STRING "Directory for the binaries")
        set(RTTR_DATADIR "${CMAKE_INSTALL_DATAROOTDIR}/s25rttr" CACHE STRING "Data directory")
        if("${RTTR_DATADIR}" STREQUAL "")
            # Avoid using absolute path accidentally
            set(DEFAULT_GAMEDIR "S2")
        else()
            set(DEFAULT_GAMEDIR "${RTTR_DATADIR}/S2")
        endif()
        set(RTTR_GAMEDIR "${DEFAULT_GAMEDIR}" CACHE STRING "(Original) Settlers 2 data directory")
        set(RTTR_LIBDIR "${CMAKE_INSTALL_LIBDIR}" CACHE STRING "Directory for shared binaries")

        # The default contains PROJECTNAME which would be "s25client"
        string(REPLACE "${PROJECT_NAME}" "s25rttr" DEFAULT_DOCDIR "${CMAKE_INSTALL_DOCDIR}")
        set(RTTR_DOCDIR "${DEFAULT_DOCDIR}" CACHE STRING "Directory for documentation files")
        if("${RTTR_LIBDIR}" STREQUAL "")
            # Avoid using absolute path accidentally
            set(DEFAULT_DRIVERDIR "s25rttr/driver")
        else()
            set(DEFAULT_DRIVERDIR "${RTTR_LIBDIR}/s25rttr/driver")
        endif()
        set(RTTR_EXTRA_BINDIR "${CMAKE_INSTALL_LIBEXECDIR}/s25rttr" CACHE STRING "Path to binaries used internally")
        set(RTTR_DRIVERDIR "${DEFAULT_DRIVERDIR}" CACHE STRING "Directory for driver objects")
    endif()
endif()

set(RTTR_SRCDIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "Path to source files. Used for shell scripts and configure")
set(RTTR_S2_PLACEHOLDER_PATH "${CMAKE_BINARY_DIR}/${RTTR_GAMEDIR}/put your S2-Installation in here")
set(RTTR_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} CACHE INTERNAL "Used for configure scripts")

# Output directories
set(RTTR_OUTPUT_DIR ${CMAKE_BINARY_DIR})
if(CMAKE_CONFIGURATION_TYPES)
    foreach(type IN LISTS CMAKE_CONFIGURATION_TYPES)
        string(TOUPPER ${type} _UPPER_TYPE)
        set(RTTR_OUTPUT_DIR_${_UPPER_TYPE} ${RTTR_OUTPUT_DIR}/${type})
    endforeach()
elseif(CMAKE_BUILD_TYPE)
    set(validBuildTypes Debug Release RelWithDebInfo MinSizeRel)
    if(NOT CMAKE_BUILD_TYPE STREQUAL "" AND NOT CMAKE_BUILD_TYPE IN_LIST validBuildTypes)
        message(FATAL_ERROR "CMAKE_BUILD_TYPE set to invalid value '${CMAKE_BUILD_TYPE}'. Valid are '' (empty) or ${validBuildTypes}")
    endif()
    string(TOUPPER ${CMAKE_BUILD_TYPE} _UPPER_TYPE)
    set(RTTR_OUTPUT_DIR_${_UPPER_TYPE} ${RTTR_OUTPUT_DIR})
endif()

# Helper function to set output dirs for all configurations
# output_type is RUNTIME, LIBRARY, ...
# subDir is the subdirectory in RTTR_OUTPUT_DIR_XXX
macro(rttr_set_output_dir output_type subDir)
    if(CMAKE_CONFIGURATION_TYPES)
        foreach(type IN LISTS CMAKE_CONFIGURATION_TYPES)
            string(TOUPPER ${type} _UPPER_TYPE)
            set(CMAKE_${output_type}_OUTPUT_DIRECTORY_${_UPPER_TYPE} ${RTTR_OUTPUT_DIR_${_UPPER_TYPE}}/${subDir})
        endforeach()
    elseif(CMAKE_BUILD_TYPE)
        string(TOUPPER ${CMAKE_BUILD_TYPE} _UPPER_TYPE)
        set(CMAKE_${output_type}_OUTPUT_DIRECTORY_${_UPPER_TYPE} ${RTTR_OUTPUT_DIR}/${subDir})
    else()
        set(CMAKE_${output_type}_OUTPUT_DIRECTORY ${RTTR_OUTPUT_DIR}/${subDir})
    endif()
endmacro()
rttr_set_output_dir(RUNTIME ${RTTR_BINDIR})
rttr_set_output_dir(LIBRARY ${RTTR_LIBDIR})

################################################################################
# Definitions and flags
################################################################################

include(CheckStructHasMember)
check_struct_has_member("struct timespec" tv_sec time.h HAVE_STRUCT_TIMESPEC)
include(CheckIncludeFile)
check_include_file(valgrind/memcheck.h HAVE_MEMCHECK_H)
if(HAVE_MEMCHECK_H)
    add_definitions(-DHAVE_MEMCHECK_H)
endif()

# Code coverage
include(RttrCoverageCfg)
include(EnableSanitizers)

option(RTTR_USE_SYSTEM_LIBS "Default to using system-wide installed libs for external dependencies that have known system versions." OFF)

################################################################################
# Configure files
################################################################################

if(NOT WIN32)
    include(ConfigureExecutable)
    configure_executable("${CMAKE_CURRENT_SOURCE_DIR}/start.sh.cmake" . start.sh)
endif()

file(WRITE "${RTTR_S2_PLACEHOLDER_PATH}" "put your S2-Installation in here")

################################################################################
# Set variables used by the release scripts and the updater:
#   PLATFORM_NAME in lowercase (windows, linux,...)
#   PLATFORM_ARCH: i386, x86_64, universal
################################################################################

string(TOLOWER ${CMAKE_SYSTEM_NAME} PLATFORM_NAME)

# Set PLATFORM_ARCH
if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(PLATFORM_ARCH "x86_64")
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(PLATFORM_ARCH "i386")
    else()
        message(FATAL_ERROR "Unknown architecture for sizeof(void*)=${CMAKE_SIZEOF_VOID_P}")
    endif()
else()
    if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "i.86")
        set(PLATFORM_ARCH "i386")
    elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "amd64|AMD64|x86_64")
        set(PLATFORM_ARCH "x86_64")
    elseif(("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "universal" OR "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "") AND APPLE)
        set(PLATFORM_ARCH "universal")
    else()
        message(STATUS "Unofficial build with processor arch: ${CMAKE_SYSTEM_PROCESSOR}. Skipping release scripts and updater.")
        unset(PLATFORM_ARCH)
        set(RTTR_BUILD_UPDATER OFF CACHE INTERNAL "Disabled")
    endif()
endif()

################################################################################
# Boost
################################################################################

include(RttrBoostCfg)
include(RttrTestingCfg)

set(rttrContribBoostDir ${CMAKE_CURRENT_SOURCE_DIR}/contrib/boost)
if(EXISTS ${rttrContribBoostDir} AND IS_DIRECTORY ${rttrContribBoostDir})
    set(BOOST_ROOT ${rttrContribBoostDir} CACHE PATH "Path to find boost at")
endif()

set(BoostPackages filesystem iostreams locale program_options)
if(BUILD_TESTING)
    # Avoid having to search for this in all tests speeding up configure
    list(APPEND BoostPackages unit_test_framework)
endif()

find_package(Boost 1.71 COMPONENTS ${BoostPackages})
if(NOT Boost_FOUND)
    message(FATAL_ERROR "You have to install boost (>=1.71) into contrib/boost or set (as CMake or environment variable) "
    "BOOST_ROOT (currently: '${BOOST_ROOT}', Environment: '$ENV{BOOST_ROOT}'), "
    "BOOST_INCLUDEDIR (currently: '${BOOST_INCLUDEDIR}', Environment: '$ENV{BOOST_INCLUDEDIR}') "
    "since cmake was unable to find boost!")
endif()

option(RTTR_USE_SYSTEM_BOOST_NOWIDE "Use system installed Boost.Nowide. Fails if not found!" "${RTTR_USE_SYSTEM_LIBS}")

if(Boost_VERSION_MINOR GREATER_EQUAL "74" OR RTTR_USE_SYSTEM_BOOST_NOWIDE)
    # Boost 1.73 contains Boost.Nowide, 1.74 a required fix
    find_package(Boost COMPONENTS nowide)
endif()
# Prefer system version, either as part of Boost or preinstalled
if(NOT Boost_NOWIDE_FOUND)
    if(RTTR_USE_SYSTEM_BOOST_NOWIDE)
        set(bnw_required REQUIRED)
    else()
        set(bnw_required QUIET)
    endif()
    find_package(boost_nowide 11.0.0 ${bnw_required})
    # Fallback in libutil, but find_package must be done here so all subprojects can use it
endif()

add_subdirectory(external)
add_subdirectory(libs)
add_subdirectory(extras)

find_package(Gettext REQUIRED)
file(GLOB RTTR_PO_FILES external/languages/*.po)
set(RTTR_TRANSLATION_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/gen/languages) # Used by copyDepsToBuildDir
gettext_create_translations(external/languages/rttr.pot ALL
    DESTINATION ${RTTR_TRANSLATION_OUTPUT}
    INSTALL_DESTINATION ${RTTR_DATADIR}/RTTR/languages
    FILES ${RTTR_PO_FILES}
)

# include Test
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

# Copy the RTTR folder with the updated translations to the directory of the client.
configure_file(copyDepsToBuildDir.cmake . @ONLY)
add_custom_command(TARGET translations POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -D CMAKE_BUILD_TYPE=$<CONFIG> -P ${CMAKE_CURRENT_BINARY_DIR}/copyDepsToBuildDir.cmake)

if(ClangFormat_FOUND)
    add_clangformat_folder(${CMAKE_CURRENT_SOURCE_DIR}/libs TRUE)
    add_clangformat_folder(${CMAKE_CURRENT_SOURCE_DIR}/extras TRUE)
    add_clangformat_folder(${CMAKE_CURRENT_SOURCE_DIR}/tests TRUE)
    add_clangformat_target("file")
endif()

################################################################################
# Install
################################################################################

# RTTR directory, but exclude language input files
install(DIRECTORY "data/RTTR" DESTINATION "${RTTR_DATADIR}"
        PATTERN ".*" EXCLUDE
)

# Documentation. TODO: Avoid recursive entries?
if(NOT WIN32)
    install(DIRECTORY "data/RTTR/texte/" DESTINATION "${RTTR_DOCDIR}" FILES_MATCHING PATTERN "*.txt")
endif()

# Placeholder for S2 installation
option(RTTR_INSTALL_PLACEHOLDER "Install a placeholder file to the location the S2 game files should be copied to." ON)
mark_as_advanced(RTTR_INSTALL_PLACEHOLDER)
if(RTTR_INSTALL_PLACEHOLDER)
    install(FILES "${RTTR_S2_PLACEHOLDER_PATH}" DESTINATION "${RTTR_GAMEDIR}")
endif()

################################################################################
# Postbuild
################################################################################

if(RTTR_BUNDLE)
    add_subdirectory("tools/release")
endif()

################################################################################
# Packing
################################################################################

MESSAGE(STATUS "RTTR Version will be: ${RTTR_VERSION}-${RTTR_REVISION}")

set(CPACK_GENERATOR "TBZ2")
if(WIN32)
    set(CPACK_GENERATOR "ZIP")
endif()

set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_PACKAGE_NAME "s25rttr")
set(CPACK_PACKAGE_VENDOR "Settlers Freaks")

set(CPACK_PACKAGE_VERSION "${RTTR_VERSION}-${RTTR_REVISION}")
set(CPACK_SYSTEM_NAME "${PLATFORM_NAME}.${PLATFORM_ARCH}")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}-${CPACK_SYSTEM_NAME}")

set(CPACK_SOURCE_GENERATOR "${CPACK_GENERATOR}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}")

set(CPACK_IGNORE_FILES "\\\\.a;/\\\\.ccache/;/\\\\.git/;${RTTR_OUTPUT_DIR};${PROJECT_SOURCE_DIR}/build.*")
set(CPACK_SOURCE_IGNORE_FILES "${CPACK_IGNORE_FILES}")
set(CPACK_BUILD_SOURCE_DIRS "${PROJECT_SOURCE_DIR}")

set(CPACK_STRIP_FILES ON)

if(APPLE)
    set(CPACK_PACKAGING_INSTALL_PREFIX "/s25client.app")
else()
    set(CPACK_PACKAGING_INSTALL_PREFIX "/s25rttr_${RTTR_VERSION}")
endif()

include(CPack)
