cmake_minimum_required(VERSION 3.8)

if(NOT MSVC)
    #Check for ccache
    if(APPLE)
        find_program(CCACHE_PROGRAM NAMES ccache-lipo ccache)
    else()
        find_program(CCACHE_PROGRAM ccache)
    endif()
    mark_as_advanced(CCACHE_PROGRAM)
    if(CCACHE_PROGRAM)
        message(STATUS "Using ccache(${CCACHE_PROGRAM}) to speed up builds")
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
    endif()
endif()

PROJECT(s25client)

if(NOT CMAKE_CXX_COMPILE_FEATURES AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.4")
    message(STATUS "Compile features for < GCC 4.4 are not supported by CMake. Emulate those")
    set(CMAKE_C_COMPILE_FEATURES "c_std_90;c_function_prototypes;c_variadic_macros" CACHE INTERNAL "")
    set(CMAKE_CXX_COMPILE_FEATURES "cxx_std_98;cxx_variadic_macros" CACHE INTERNAL "")
endif()

################################################################################

message(STATUS "Using CMake ${CMAKE_VERSION}")

IF (DEFINED CMAKE_TOOLCHAIN_FILE)
	MESSAGE(STATUS "Used Toolchain definition file '${CMAKE_TOOLCHAIN_FILE}'")
ENDIF()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules" "${CMAKE_SOURCE_DIR}/external/libutil/cmake")

# Just include required
INCLUDE(EnsureOutOfSourceBuild)

include(CheckStructHasMember)
INCLUDE(CheckIncludeFile)

IF(APPLE)
	SET(CMAKE_INSTALL_RPATH "@executable_path" "@executable_path/../Frameworks")
ELSEIF(NOT WIN32)
	SET(CMAKE_INSTALL_RPATH "\$ORIGIN" "\$ORIGIN/../lib")
ENDIF()

################################################################################
# Include platform specific config files
################################################################################

STRING(TOLOWER ${CMAKE_SYSTEM_NAME} LOWER_SYSTEM_NAME)
IF (CMAKE_CROSSCOMPILING)
	MESSAGE(STATUS "Configuring for cross-compiling to ${CMAKE_SYSTEM} on ${CMAKE_SYSTEM_PROCESSOR}")
	#Include cross-compile platform config
    set(_config "cmake/c.${LOWER_SYSTEM_NAME}.cmake")
	IF (EXISTS "${CMAKE_CURRENT_LIST_DIR}/${_config}")
		MESSAGE(STATUS "Using cross-platform config ${_config}")
		INCLUDE("${_config}")
	ENDIF()
ELSE()
	MESSAGE(STATUS "Configuring for native compiling to ${CMAKE_SYSTEM} on ${CMAKE_SYSTEM_PROCESSOR}")
ENDIF()

#Include generic platform config
set(_config "cmake/${LOWER_SYSTEM_NAME}.cmake")
IF (EXISTS "${CMAKE_CURRENT_LIST_DIR}/${_config}")
    MESSAGE(STATUS "Using platform config ${_config}")
    INCLUDE("${_config}")
ENDIF()

OPTION(RTTR_ENABLE_OPTIMIZATIONS "Build with optimizing flags (such as -O2 and -ffast-math added to CFLAGS and CXXFLAGS)" ON)
if(RTTR_ENABLE_OPTIMIZATIONS)
    include("cmake/optimizations.cmake")
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -ggdb")
endif()

include(CheckGitSubmodules)
check_git_submodules()

################################################################################
# ClangFormat
################################################################################

find_package(ClangFormat 5.0.0)

################################################################################
# Set paths.
# Note: Mixing relative and absolute paths is not recommended!
# If RTTR_BINDIR is absolute then relative paths are relative to CMAKE_INSTALL_PREFIX
# Else a prefix path is calculated based on RTTR_BINDIR and the current executable path so that we have <PrefixPath>/RTTR_BINDIR
# In this case all relative paths are relative to that prefix path
# Example: RTTR_BINDIR="bin", RTTR_DATADIR="data" executed in "/foo/bar/bin/rttr.exe" -> FULL_DATADIR=/foo/bar/data
################################################################################

IF (WIN32)
	# Windows uses one directory only
	SET(RTTR_BINDIR "." CACHE INTERNAL "")
	SET(RTTR_DATADIR "." CACHE INTERNAL "")
	SET(RTTR_GAMEDIR "." CACHE INTERNAL "")
	SET(RTTR_LIBDIR "." CACHE INTERNAL "")
	SET(RTTR_DOCDIR "." CACHE INTERNAL "")
	SET(RTTR_EXTRA_BINDIR "./RTTR" CACHE INTERNAL "")
	SET(RTTR_DRIVERDIR "./driver" CACHE INTERNAL "")
ELSE()
	if(APPLE)
		OPTION(RTTR_OSX_BUNDLE "Create an Apple/OSX bundle directory (=clickable application)" ON)
	endif()

	IF(APPLE AND RTTR_OSX_BUNDLE)
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
		SET(RTTR_BINDIR "${CMAKE_INSTALL_BINDIR}" CACHE STRING "Directory for the binaries")
		SET(RTTR_DATADIR "${CMAKE_INSTALL_DATAROOTDIR}/s25rttr" CACHE STRING "Data directory")
		if("${RTTR_DATADIR}" STREQUAL "")
			# Avoid using absolute path accidentally
			SET(DEFAULT_GAMEDIR "S2")
		else()
			SET(DEFAULT_GAMEDIR "${RTTR_DATADIR}/S2")
		endif()
		SET(RTTR_GAMEDIR "${DEFAULT_GAMEDIR}" CACHE STRING "(Original) Settlers 2 data directory")
		SET(RTTR_LIBDIR "${CMAKE_INSTALL_LIBDIR}" CACHE STRING "Directory for shared binaries")
		# The default contains PROJECTNAME which would be "s25client"
		string(REPLACE "${PROJECT_NAME}" "s25rttr" DEFAULT_DOCDIR "${CMAKE_INSTALL_DOCDIR}")
		SET(RTTR_DOCDIR "${DEFAULT_DOCDIR}" CACHE STRING "Directory for documentation files")
		if("${RTTR_LIBDIR}" STREQUAL "")
			# Avoid using absolute path accidentally
			SET(DEFAULT_DRIVERDIR "driver")
		else()
			SET(DEFAULT_DRIVERDIR "${RTTR_LIBDIR}/driver")
		endif()
		SET(RTTR_EXTRA_BINDIR "${CMAKE_INSTALL_LIBEXECDIR}/s25rttr" CACHE STRING "Path to binaries used internally")
		SET(RTTR_DRIVERDIR "${DEFAULT_DRIVERDIR}" CACHE STRING "Directory for driver objects")
	ENDIF()
ENDIF()

SET(RTTR_SRCDIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "Path to source files. Used for shell scripts and configure")
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

# Do this before adding -Werror!
check_struct_has_member("struct timespec" tv_sec time.h HAVE_STRUCT_TIMESPEC)
ADD_DEFINITIONS(-DBOOST_PP_VARIADICS=1)

CHECK_INCLUDE_FILE(valgrind/memcheck.h HAVE_MEMCHECK_H)
IF(HAVE_MEMCHECK_H)
	ADD_DEFINITIONS(-DHAVE_MEMCHECK_H)
ENDIF()

# Clang bug workaround
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	try_compile(CHECK_CLANG_INLINE "${CMAKE_CURRENT_BINARY_DIR}/cxx" "${CMAKE_SOURCE_DIR}/cmake/checkclang.cpp")
	if(NOT ${CHECK_CLANG_INLINE})
		add_definitions("-D__extern_always_inline=extern __always_inline __attribute__ ((__gnu_inline__))")
	endif()
endif()

################################################################################
# Code coverage
################################################################################

# VS does not support coverage analysis
if(NOT MSVC)
	option(RTTR_ENABLE_COVERAGE "Generate coverage build" OFF)
	if(RTTR_ENABLE_COVERAGE)
		if(NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
			message(FATAL_ERROR "Coverage requires a debug build or you get false positives")
		endif()
		# Note: "--coverage" instead of "-coverage" makes this work with ccache
		add_compile_options(-O0 --coverage)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
		if(CMAKE_COMPILER_IS_GNUCXX)
			# Inlining makes the coverage statistic much harder to read and may lead to lots of partials
			# However expect a slowdown!
			message(STATUS "Enabled no-inline for better coverage statistics")
			add_compile_options(-fno-inline -fno-inline-small-functions -fno-default-inline)
		else()
			message(STATUS "Using default inlining which probably influences coverage statistics")
		endif()
	endif()
endif()

################################################################################
# Configure files
################################################################################

IF (NOT WIN32)
    include(ConfigureExecutable)
    configure_executable("${CMAKE_CURRENT_SOURCE_DIR}/start.sh.cmake" . start.sh)
ENDIF()

file(COPY "bugle" DESTINATION ".")
file(WRITE "${RTTR_S2_PLACEHOLDER_PATH}" "put your S2-Installation in here")

# Building the updater when using non-default paths (especially absolute paths)
# is not recommended as it most likely won't work
set(RTTR_BUILD_UPDATER_DEF ON)
foreach(curDirVar BINDIR DATADIR LIBDIR DOCDIR DRIVERDIR)
	if(IS_ABSOLUTE ${RTTR_${curDirVar}})
		set(RTTR_BUILD_UPDATER_DEF OFF)
	endif()
endforeach()
option(RTTR_BUILD_UPDATER "Build auto-updater. Not advised when changing the default paths as the updater is configured specifically for the official builds." ${RTTR_BUILD_UPDATER_DEF})
################################################################################
# Set variables used by the release scripts and the updater:
#   PLATFORM_NAME in lowercase (windows, linux,...)
#   PLATFORM_ARCH: i386, x86_64, universal
################################################################################

STRING(TOLOWER ${CMAKE_SYSTEM_NAME} PLATFORM_NAME)

# Set PLATFORM_ARCH
IF(WIN32)
	IF (CMAKE_SIZEOF_VOID_P EQUAL 8)
		SET(PLATFORM_ARCH "x86_64")
	ELSEIF (CMAKE_SIZEOF_VOID_P EQUAL 4)
		SET(PLATFORM_ARCH "i386")
	ELSE()
		MESSAGE(FATAL_ERROR "Unknown platform for sizeof(void*)=${CMAKE_SIZEOF_VOID_P}")
	ENDIF ()
ELSE()
	if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "i.86")
		SET(PLATFORM_ARCH "i386")
	elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "amd64|AMD64|x86_64")
		SET(PLATFORM_ARCH "x86_64")
	ELSEIF(("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "universal" OR "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "") AND APPLE)
		SET(PLATFORM_ARCH "universal")
	ELSE()
		MESSAGE(STATUS "Unofficial build with processor arch: ${CMAKE_SYSTEM_PROCESSOR}. Skipping release scripts and updater.")
        unset(PLATFORM_ARCH)
        set(RTTR_BUILD_UPDATER OFF CACHE INTERNAL "Disabled")
	ENDIF()
ENDIF()

################################################################################
# Boost
################################################################################

include(RttrBoostCfg)

IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/boost AND IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/boost)
	SET(BOOST_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/boost CACHE PATH "Path to find boost at")
ENDIF()

FIND_PACKAGE(Boost 1.55.0 QUIET)
IF(NOT Boost_FOUND)
	MESSAGE(FATAL_ERROR "You have to install boost into contrib/boost or set (as CMake or environment variable) "
	"BOOST_ROOT (currently: '${BOOST_ROOT}', Environment: '$ENV{BOOST_ROOT}'), "
	"BOOST_INCLUDEDIR (currently: '${BOOST_INCLUDEDIR}', Environment: '$ENV{BOOST_INCLUDEDIR}'), "
	"since cmake was unable to find boost!")
ELSEIF(BOOST_LIBRARYDIR STREQUAL "")
	list(APPEND CMAKE_PREFIX_PATH ${Boost_INCLUDE_DIR}/stage)
	set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} PARENT_SCOPE)
ENDIF()

include(EnableBoostBackport)
enable_boost_backport()

add_subdirectory(external)
add_subdirectory(libs)
add_subdirectory(extras)

enable_testing()
add_subdirectory(tests)

find_package(Gettext REQUIRED)
FILE(GLOB RTTR_PO_FILES data/RTTR/languages/*.po)
GETTEXT_CREATE_TRANSLATIONS(data/RTTR/languages/rttr.pot ALL
							${RTTR_PO_FILES}
						   )
# Copy the RTTR folder with the updated translations to the directory of the client.
configure_file(copyDepsToBuildDir.cmake . @ONLY)
add_custom_command(TARGET translations POST_BUILD COMMAND ${CMAKE_COMMAND} -D CMAKE_BUILD_TYPE=$<CONFIG> -P ${CMAKE_CURRENT_BINARY_DIR}/copyDepsToBuildDir.cmake)

if(ClangFormat_FOUND)
	add_ClangFormat_folder(${CMAKE_CURRENT_SOURCE_DIR}/libs TRUE)
	add_ClangFormat_folder(${CMAKE_CURRENT_SOURCE_DIR}/extra TRUE)
	add_ClangFormat_folder(${CMAKE_CURRENT_SOURCE_DIR}/tests TRUE)
	add_ClangFormat_target("file")
endif()

################################################################################
# Install
################################################################################

# RTTR directory, but exclude language input files
install(DIRECTORY "data/RTTR" DESTINATION "${RTTR_DATADIR}"
		PATTERN "*.po" EXCLUDE
		PATTERN "*.pot" EXCLUDE
		PATTERN ".*" EXCLUDE
)

# Documentation. TODO: Avoid recursive entries?
IF (NOT WIN32)
	install(DIRECTORY "data/RTTR/texte/" DESTINATION "${RTTR_DOCDIR}"
			FILES_MATCHING PATTERN "*.txt"
	)
ENDIF()

# Placeholder for S2 installation
install(FILES "${RTTR_S2_PLACEHOLDER_PATH}" DESTINATION "${RTTR_GAMEDIR}")

# Start script
IF (WIN32)
	install(PROGRAMS "tools/release/bin/rttr.bat" DESTINATION "${RTTR_BINDIR}")
ELSEIF(APPLE)
	if(COMMAND rttr_create_bundle)
		rttr_create_bundle()
	endif()
ELSEIF(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" OR CYGWIN)
	install(PROGRAMS "tools/release/bin/rttr.sh" DESTINATION "${RTTR_BINDIR}")
ELSE()
	message(FATAL_ERROR "${CMAKE_SYSTEM_NAME} not supported")
ENDIF()

################################################################################
# Custom targets
################################################################################

IF(NOT CMAKE_HOST_WIN32 AND PLATFORM_ARCH)
    include(ConfigureExecutable)
    configure_executable("tools/release/prepareRelease.sh.cmake" . prepareRelease.sh)
    configure_executable("tools/release/create_release.sh.cmake" . create_release.sh)

	# Create nightly release
	ADD_CUSTOM_TARGET(create_nightly
					  COMMAND "${CMAKE_CURRENT_BINARY_DIR}/create_release.sh" nightly
	)

	# Create stable release
	ADD_CUSTOM_TARGET(create_stable
					  COMMAND "${CMAKE_CURRENT_BINARY_DIR_DIR}/create_release.sh" stable
	)
ENDIF()
