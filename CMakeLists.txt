
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)
IF(NOT CMAKE_VERSION VERSION_LESS 3.0)
	# RPath enabled by default. Rpath only supported in CMake >2.8.12
	CMAKE_POLICY(SET CMP0042 OLD)
ENDIF()
IF(NOT CMAKE_VERSION VERSION_LESS 3.1)
    # Simplified variable expansion. No change for usual variable references so use faster one
	CMAKE_POLICY(SET CMP0053 NEW)
ENDIF()

PROJECT(s25client)

################################################################################

IF (DEFINED CMAKE_TOOLCHAIN_FILE)
	MESSAGE(STATUS "Used Toolchain definition file '${CMAKE_TOOLCHAIN_FILE}'")
ENDIF()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules" "${CMAKE_SOURCE_DIR}/libutil/cmake/Modules")

set(RTTR_CONTRIB_DIR ${CMAKE_SOURCE_DIR}/contrib)
list(APPEND CMAKE_PREFIX_PATH "${RTTR_CONTRIB_DIR}")

# Fill RTTR_CONTRIB_DIR_VS and append to CMAKE_PREFIX_PATH
# Set CMAKE_LIBRARY_ARCHITECTURE
IF (MSVC)
	SET(RTTR_CONTRIB_DIR_VS "${RTTR_CONTRIB_DIR}/full-contrib-msvc" CACHE PATH "Path to base dir from contrib package")
	IF(NOT EXISTS ${RTTR_CONTRIB_DIR_VS} OR NOT IS_DIRECTORY ${RTTR_CONTRIB_DIR_VS})
		MESSAGE(FATAL_ERROR "You have to extract contrib/full-contrib-msvc.rar to ${RTTR_CONTRIB_DIR_VS} first!")
	ENDIF()
	IF(MSVC_VERSION GREATER 1800) # MSVC 2015
		list(APPEND CMAKE_PREFIX_PATH "${RTTR_CONTRIB_DIR_VS}/VS2015")
	ENDIF()
	list(APPEND CMAKE_PREFIX_PATH ${RTTR_CONTRIB_DIR_VS} ${RTTR_CONTRIB_DIR_VS}/buildTools)

	IF(CMAKE_SIZEOF_VOID_P EQUAL 4)
		SET(CMAKE_LIBRARY_ARCHITECTURE "x86" CACHE INTERNAL "")
	ELSE()
		SET(CMAKE_LIBRARY_ARCHITECTURE "x64" CACHE INTERNAL "")
	ENDIF()
	MESSAGE(STATUS "Building for MSVC: ${CMAKE_LIBRARY_ARCHITECTURE}")
ENDIF()

INCLUDE(CMakeMacroSetCCache)
INCLUDE(CMakeMacroAddFlags)
INCLUDE(CMakeMacroForceAddFlags)
INCLUDE(CMakeMacroCorrectLib)
INCLUDE(CMakeMacroEnsureOutOfSourceBuild)
INCLUDE(CMakeDetectOsXArchs)
include(CheckStructHasMember)
INCLUDE(CheckCXXCompilerFlag)
INCLUDE(CheckCCompilerFlag)
INCLUDE(CheckIncludeFile)

SET (RTTR_RPATH "")
IF(WIN32)
	# empty RPATH
ELSEIF(APPLE)
	SET(RTTR_RPATH "@executable_path/.." "@executable_path/../Frameworks" "@executable_path")
ELSE()
	SET(RTTR_RPATH "\$ORIGIN/../lib" "\$ORIGIN")
ENDIF()

IF( RTTR_RPATH )
	FOREACH(RPATH ${RTTR_RPATH})
		FORCE_ADD_FLAGS(CMAKE_EXE_LINKER_FLAGS -Wl,-rpath,${RPATH} )
		FORCE_ADD_FLAGS(CMAKE_MODULE_LINKER_FLAGS -Wl,-rpath,${RPATH} )
		FORCE_ADD_FLAGS(CMAKE_SHARED_LINKER_FLAGS -Wl,-rpath,${RPATH} )
	ENDFOREACH()
ENDIF()

################################################################################
# Unify variables:
#   PLATFORM_NAME in lowercase (windows, linux,...)
#   PLATFORM_ARCH: i386, x86_64, universal
################################################################################

STRING(TOLOWER ${CMAKE_SYSTEM_NAME} PLATFORM_NAME)
SET(PLATFORM_NAME ${PLATFORM_NAME} CACHE INTERNAL "Do not change")

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
	IF("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "i386" OR "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "i686")
		SET(PLATFORM_ARCH "i386")
	ELSEIF("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64" OR "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "amd64" OR "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "AMD64")
		SET(PLATFORM_ARCH "x86_64")
	ELSEIF(("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "universal" OR "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "") AND APPLE)
		SET(PLATFORM_ARCH "universal")
	ELSE()
		MESSAGE(FATAL_ERROR "Unknown processor arch: ${CMAKE_SYSTEM_PROCESSOR}")
	ENDIF()
ENDIF()
# Fill cache for post-processing tools
SET(PLATFORM_ARCH ${PLATFORM_ARCH} CACHE INTERNAL "Do not change")

################################################################################
# Include platform specific config files

IF (CMAKE_CROSSCOMPILING)
	MESSAGE(STATUS "Configuring for cross-compiling to ${CMAKE_SYSTEM} on ${PLATFORM_ARCH}")
	#Include cross-compile platform config
	IF (EXISTS "${CMAKE_CURRENT_LIST_DIR}/cmake/c.${PLATFORM_NAME}.cmake")
		MESSAGE(STATUS "Using cross-platform config cmake/c.${PLATFORM_NAME}.cmake")
		INCLUDE("cmake/c.${PLATFORM_NAME}.cmake")
	ENDIF()
ELSE (CMAKE_CROSSCOMPILING)
	MESSAGE(STATUS "Configuring for native compiling to ${CMAKE_SYSTEM} on ${PLATFORM_ARCH}")
ENDIF (CMAKE_CROSSCOMPILING)

#Include generic platform config
OPTION(RTTR_ENABLE_OPTIMIZATIONS "Build with optimizing flags (such as -O2 and -ffast-math added to CFLAGS and CXXFLAGS)" ON)
MESSAGE(STATUS "Using platform config cmake/${PLATFORM_NAME}.cmake")
INCLUDE("cmake/${PLATFORM_NAME}.cmake")

################################################################################
# Boost
################################################################################

include(RttrBoostCfg)

IF(EXISTS "${RTTR_CONTRIB_DIR}/boost" AND IS_DIRECTORY "${RTTR_CONTRIB_DIR}/boost")
	SET(BOOST_ROOT ${RTTR_CONTRIB_DIR}/boost CACHE PATH "Path to find boost at")
ENDIF()

FIND_PACKAGE(Boost 1.55.0)
IF(NOT Boost_FOUND)
	MESSAGE(FATAL_ERROR "You have to install boost into contrib/boost or set (as CMake or environment variable) "
    "BOOST_ROOT (currently: '${BOOST_ROOT}', Environment: '$ENV{BOOST_ROOT}'), "
    "BOOST_INCLUDEDIR (currently: '${BOOST_INCLUDEDIR}', Environment: '$ENV{BOOST_INCLUDEDIR}'), "
    "since cmake was unable to find boost!")
ELSEIF("${BOOST_LIBRARYDIR}" STREQUAL "")
	list(APPEND CMAKE_PREFIX_PATH "${Boost_INCLUDE_DIR}/stage")
ENDIF()

SET(TMP_BOOST_VERSION "${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}")
IF(TMP_BOOST_VERSION VERSION_LESS 1.56)
	IF(MSVC AND MSVC_VERSION EQUAL 1800)
		# See https://svn.boost.org/trac/boost/ticket/9332
		MESSAGE(FATAL_ERROR "Boost 1.55 contains a bug so that it does not work with MSVC 2013. Use a newer boost version or a different Visual Studio")
	ENDIF()
	MESSAGE(STATUS "Boost version smaller than 1.56 detected. Using backport 1.55-1.56")
	INCLUDE_DIRECTORIES("${RTTR_CONTRIB_DIR}/backport/boost_1.55-1.56")
ENDIF()
IF(TMP_BOOST_VERSION VERSION_LESS 1.58)
	MESSAGE(STATUS "Boost version smaller than 1.58 detected. Using backport 1.56-1.58")
	INCLUDE_DIRECTORIES("${RTTR_CONTRIB_DIR}/backport/boost_1.56-1.58")
ELSE()
	MESSAGE(STATUS "Boost ${Boost_VERSION} detected. No backport required")
ENDIF()
UNSET(TMP_BOOST_VERSION)

################################################################################
# ClangFormat
################################################################################

find_package(ClangFormat 5.0.0)

################################################################################
# Bzip2 sources
################################################################################

if(WIN32)
    set(bzip2ContribDir "${RTTR_CONTRIB_DIR}/bzip2-1.0.6")
	IF(IS_DIRECTORY "${bzip2ContribDir}" )
		SET(SOURCES_BZIP
			${bzip2ContribDir}/blocksort.c
			${bzip2ContribDir}/bzlib.c
			${bzip2ContribDir}/compress.c
			${bzip2ContribDir}/crctable.c
			${bzip2ContribDir}/decompress.c
			${bzip2ContribDir}/huffman.c
			${bzip2ContribDir}/randtable.c
		)
        add_library(bzip2 STATIC ${SOURCES_BZIP})
        set(BZIP2_FOUND TRUE)
        set(BZIP2_LIBRARIES bzip2)
        set(BZIP2_INCLUDE_DIR ${bzip2ContribDir})
	ENDIF()
ENDIF()

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
	SET(RTTR_DRIVERDIR "./driver" CACHE INTERNAL "")
ELSEIF(APPLE)
	# Apple wants an Apple Bundle
	SET(RTTR_BINDIR "./s25client.app/Contents/MacOS/bin" CACHE INTERNAL "")
	SET(RTTR_DATADIR "./s25client.app/Contents/MacOS/share/s25rttr" CACHE INTERNAL "")
	SET(RTTR_GAMEDIR "./s25client.app/Contents/MacOS/share/s25rttr/S2" CACHE INTERNAL "")
	SET(RTTR_LIBDIR "./s25client.app/Contents/MacOS" CACHE INTERNAL "")
	SET(RTTR_DOCDIR "./s25client.app/Contents/MacOS/share/doc" CACHE INTERNAL "")
	SET(RTTR_DRIVERDIR "./s25client.app/Contents/MacOS/share/s25rttr/driver" CACHE INTERNAL "")
ELSE()
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
	SET(RTTR_DRIVERDIR "${DEFAULT_DRIVERDIR}" CACHE STRING "Directory for driver objects")
ENDIF()

SET(RTTR_SRCDIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "Path to source files. Used for shell scripts and configure")
set(RTTR_S2_PLACEHOLDER_PATH "${CMAKE_BINARY_DIR}/${RTTR_GAMEDIR}/put your S2-Installation in here")
set(RTTR_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} CACHE INTERNAL "Used for configure scripts")

# Output directories
set(RTTR_OUTPUT_DIR ${CMAKE_BINARY_DIR})
if(CMAKE_CONFIGURATION_TYPES)
    set(RTTR_OUTPUT_DIR_DEBUG ${RTTR_OUTPUT_DIR}/Debug)
    set(RTTR_OUTPUT_DIR_RELEASE ${RTTR_OUTPUT_DIR}/Release)
    set(RTTR_OUTPUT_DIR_RELWITHDEBINFO ${RTTR_OUTPUT_DIR}/RelWithDebInfo)
    set(RTTR_OUTPUT_DIR_MINSIZEREL ${RTTR_OUTPUT_DIR}/MinSizeRel)
else()
    set(RTTR_OUTPUT_DIR_DEBUG ${RTTR_OUTPUT_DIR})
    set(RTTR_OUTPUT_DIR_RELEASE ${RTTR_OUTPUT_DIR})
    set(RTTR_OUTPUT_DIR_RELWITHDEBINFO ${RTTR_OUTPUT_DIR})
    set(RTTR_OUTPUT_DIR_MINSIZEREL ${RTTR_OUTPUT_DIR})
endif()

# Helper function to set output dirs. See below for usage example
macro(rttr_set_output_dir type subDir)
    set(CMAKE_${type}_OUTPUT_DIRECTORY ${RTTR_OUTPUT_DIR}/${subDir})
    set(CMAKE_${type}_OUTPUT_DIRECTORY_DEBUG ${RTTR_OUTPUT_DIR_DEBUG}/${subDir})
    set(CMAKE_${type}_OUTPUT_DIRECTORY_RELEASE ${RTTR_OUTPUT_DIR_RELEASE}/${subDir})
    set(CMAKE_${type}_OUTPUT_DIRECTORY_RELWITHDEBINFO ${RTTR_OUTPUT_DIR_RELWITHDEBINFO}/${subDir})
    set(CMAKE_${type}_OUTPUT_DIRECTORY_MINSIZEREL ${RTTR_OUTPUT_DIR_MINSIZEREL}/${subDir})
endmacro()
rttr_set_output_dir(RUNTIME ${RTTR_BINDIR})
rttr_set_output_dir(LIBRARY ${RTTR_LIBDIR})

# ensure out of source build
ENSURE_OUT_OF_SOURCE_BUILD("${PROJECT_NAME} requires an out of source build. Please go to the build directory and run './cmake.sh [options]' there.")

################################################################################
# Definitions and flags
################################################################################

# Do this before adding -Werror!
check_struct_has_member("struct timespec" tv_sec time.h HAVE_STRUCT_TIMESPEC)

MACRO(CheckAndAddFlag flag)
	# We cannot check for -Wno-foo as this won't throw a warning so we must check for the -Wfoo option directly
	# http://stackoverflow.com/questions/38785168/cc1plus-unrecognized-command-line-option-warning-on-any-other-warning
	STRING(REGEX REPLACE "^-Wno-" "-W" checkedFlag ${flag})
	SET(VarName ${checkedFlag})
	STRING(REPLACE "+" "X" VarName ${VarName})
	STRING(REGEX REPLACE "[=-]" "_" VarName ${VarName})
    CHECK_CXX_COMPILER_FLAG(${checkedFlag} CXX_FLAG_${VarName}_SUPPORTED)
    CHECK_C_COMPILER_FLAG(${checkedFlag} C_FLAG_${VarName}_SUPPORTED)
	IF (CXX_FLAG_${VarName}_SUPPORTED)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
	ENDIF ()
	IF (C_FLAG_${VarName}_SUPPORTED)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${flag}")
	ENDIF ()
	unset(VarName)
	unset(checkedFlag)
ENDMACRO()

OPTION(RTTR_ENABLE_WERROR "Build with warnings turned into errors" ON)
IF (MSVC)
	add_compile_options(/W3)
	add_compile_options(/MP) # parallel compilation
	# Signed/Unsigned operations
	add_compile_options(/w34389)
	IF(RTTR_ENABLE_WERROR)
		add_compile_options(/WX) # warning = error
	ENDIF()
ELSE ()
	CheckAndAddFlag("-Wall")
	IF(RTTR_ENABLE_WERROR)
		CheckAndAddFlag("-Werror")
	ENDIF()
    CheckAndAddFlag("-pedantic")
    CheckAndAddFlag("-Wpedantic")
    CheckAndAddFlag("-Wparentheses")
    CheckAndAddFlag("-Wno-error=type-limits")
    CheckAndAddFlag("-Wfloat-conversion")
    CheckAndAddFlag("-Wno-long-long")
    CheckAndAddFlag("-Wno-deprecated-register")
    CheckAndAddFlag("-Wno-unknown-pragmas")
    CheckAndAddFlag("-Wno-c++11-extensions")
    CheckAndAddFlag("-fno-strict-aliasing")
    CheckAndAddFlag("-Qunused-arguments")

    CHECK_CXX_SOURCE_COMPILES("
        #if __cplusplus >= 201103L
        int main() {}
        #endif" COMPILER_IN_CXX11_MODE)
    IF(COMPILER_IN_CXX11_MODE)
        CheckAndAddFlag("-Wsuggest-override")
        CheckAndAddFlag("-Wno-error=suggest-override")
    else()
    	ADD_DEFINITIONS(-Doverride=)
    ENDIF(COMPILER_IN_CXX11_MODE)

    # Variadic macros are part of C99 but supported by all big compilers in C++03
    CheckAndAddFlag("-Wno-variadic-macros")
    CheckAndAddFlag("-Wno-c99-extensions")
ENDIF (MSVC)

ADD_DEFINITIONS(-DBOOST_PP_VARIADICS=1)

IF (WIN32)
	# Enforce unicode
	ADD_DEFINITIONS(-DUNICODE)
	ADD_DEFINITIONS(-D_UNICODE)
ENDIF()

CHECK_INCLUDE_FILE(valgrind/memcheck.h HAVE_MEMCHECK_H)
IF(HAVE_MEMCHECK_H)
    ADD_DEFINITIONS(-DHAVE_MEMCHECK_H)
ENDIF()

# Clang bug workaround
if(CMAKE_CXX_COMPILER_ID STREQUAL Clang)
    try_compile(CHECK_CLANG_INLINE "${CMAKE_CURRENT_BINARY_DIR}/cxx" "${CMAKE_SOURCE_DIR}/cmake/checkclang.cpp")
    if(NOT ${CHECK_CLANG_INLINE})
        add_definitions("-D__extern_always_inline=extern __always_inline __attribute__ ((__gnu_inline__))")
    endif()

    # Some workaround flags for execssive warnings in clang
    CheckAndAddFlag(-Wno-unused-local-typedefs)
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
		add_flags(CMAKE_CXX_FLAGS_DEBUG -O0 --coverage)
		if(CMAKE_COMPILER_IS_GNUCXX)
			# Inlining makes the coverage statistic much harder to read and may lead to lots of partials
			# However expect a slowdown!
			message(STATUS "Enabled no-inline for better coverage statistics")
			add_flags(CMAKE_CXX_FLAGS_DEBUG -fno-inline -fno-inline-small-functions -fno-default-inline)
		else()
			message(STATUS "Using default inlining which probably influences coverage statistics")
		endif()
	endif()
endif()

################################################################################
# Configure files
################################################################################

# Make sure this exists
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/CMakeFiles)
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/build_paths.h.cmake"
			   build_paths.h @ONLY)
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/release/prepareRelease.sh.cmake"
		   CMakeFiles/prepareRelease.sh @ONLY)
file(COPY ${CMAKE_BINARY_DIR}/CMakeFiles/prepareRelease.sh
	DESTINATION "."
	FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
	GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/copyDepsToBuildDir.cmake CMakeFiles @ONLY)

IF (NOT WIN32)
	CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/start.sh.cmake"
			   ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/start.sh @ONLY)
	file(COPY ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/start.sh
		DESTINATION "."
		FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
		GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
ENDIF()
file(COPY "bugle" DESTINATION ".")
file(WRITE "${RTTR_S2_PLACEHOLDER_PATH}" "put your S2-Installation in here")

# add build directory to compiler search path for configured includes
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})

enable_testing()

# Add dlls from contrib folder
if(MSVC)
    SET(CONTRIB_DLL_DIR "${RTTR_CONTRIB_DIR_VS}/bin/${CMAKE_LIBRARY_ARCHITECTURE}")
    if(NOT EXISTS "${CONTRIB_DLL_DIR}/libcurl.dll")
        MESSAGE(WARNING "Folder with DLLs not found in ${CONTRIB_DLL_DIR}. You may not be able to execute directly from VS")
    else()
        FILE(GLOB CONTRIB_DLLS ${CONTRIB_DLL_DIR}/*.dll)
        include(GatherDll)
        gather_dll_add(${CONTRIB_DLLS})
    ENDIF()
endif()

################################################################################
# Include projects
################################################################################

# Building the updater when using non-default paths (especially absolute paths)
# is not recommended as it most likely won't work
set(RTTR_BUILD_UPDATER_DEF ON)
foreach(curDirVar BINDIR DATADIR LIBDIR DOCDIR DRIVERDIR)
	if(IS_ABSOLUTE ${RTTR_${curDirVar}})
		set(RTTR_BUILD_UPDATER_DEF OFF)
	endif()
endforeach()
option(RTTR_BUILD_UPDATER "Build auto-updater. Not advised when changing the default paths as the updater is configured specifically for the official builds." ${RTTR_BUILD_UPDATER_DEF})

# Add this first as it defines dependencies for other projects (e.g. nowide and utf8)
ADD_SUBDIRECTORY(libutil)

# Then we need the version generator so we can depend on it
ADD_SUBDIRECTORY(version)

ADD_CUSTOM_TARGET(updateversion ALL
				  COMMAND $<TARGET_FILE:version> "${CMAKE_SOURCE_DIR}"
				  DEPENDS version
				  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)

# And the rest

ADD_SUBDIRECTORY(libendian)
ADD_SUBDIRECTORY(mygettext)
ADD_SUBDIRECTORY(libsiedler2)
# Build lobby client
SET(RTTR_BUILD_LOBBY_CLIENT TRUE)
ADD_SUBDIRECTORY(liblobby)
ADD_SUBDIRECTORY(s-c)
if(RTTR_BUILD_UPDATER)
    ADD_SUBDIRECTORY(s25update)
endif()

if(HAVE_STRUCT_TIMESPEC)
    add_definitions(-DHAVE_STRUCT_TIMESPEC)
endif()
add_subdirectory(common)
ADD_SUBDIRECTORY(driver)
ADD_SUBDIRECTORY(rttrConfig)
ADD_SUBDIRECTORY(src)

option(RTTR_BUILD_EDITOR "Build editor." ON)
if(RTTR_BUILD_EDITOR)
	add_subdirectory(s25edit)
endif()

if(ClangFormat_FOUND)
    add_ClangFormat_folder(${CMAKE_CURRENT_SOURCE_DIR}/driver true)
    add_ClangFormat_folder(${CMAKE_CURRENT_SOURCE_DIR}/src true)
    add_ClangFormat_target("file")
endif()

################################################################################
# Install
################################################################################

# Start script
IF (WIN32)
	install(PROGRAMS "release/bin/rttr.bat" DESTINATION "${RTTR_BINDIR}")
ELSEIF(APPLE)
	install(PROGRAMS "release/bin/macos/rttr.command" DESTINATION "${RTTR_LIBDIR}")
	install(PROGRAMS "release/bin/macos/rttr.terminal" DESTINATION "${RTTR_LIBDIR}")
	install(FILES "release/bin/macos/icon.icns" DESTINATION "${RTTR_LIBDIR}/../Resources")
	install(FILES "release/bin/macos/PkgInfo" DESTINATION "${RTTR_LIBDIR}/..")
	install(FILES "release/bin/macos/Info.plist" DESTINATION "${RTTR_LIBDIR}/..")
	IF (CMAKE_CROSSCOMPILING)
		# copy Frameworks
		LIST(GET SDL_LIBRARY 0 SDL_FRAMEWORK)
		install(DIRECTORY ${SDL_FRAMEWORK} DESTINATION "${RTTR_LIBDIR}/Frameworks"
				PATTERN "Headers" EXCLUDE
		)
		LIST(GET SDL_MIXER_LIBRARY 0 SDL_mixer_FRAMEWORK)
		install(DIRECTORY ${SDL_mixer_FRAMEWORK} DESTINATION "${RTTR_LIBDIR}/Frameworks"
				PATTERN "Headers" EXCLUDE
		)

		# copy miniupnpc
		get_filename_component(MINIUPNPC_DIR ${MINIUPNPC_LIBRARY} DIRECTORY)
		get_filename_component(MINIUPNPC_NAME ${MINIUPNPC_LIBRARY} NAME_WE)
		FILE(GLOB MINIUPNPC_DYLIBS ${MINIUPNPC_DIR}/${MINIUPNPC_NAME}*.dylib)
		FOREACH(DYLIB ${MINIUPNPC_DYLIBS} )
		MESSAGE(STATUS ${DYLIB})
			install(FILES ${DYLIB} DESTINATION "${RTTR_LIBDIR}/")
		ENDFOREACH()
	ENDIF()
ELSEIF(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" OR CYGWIN)
	install(PROGRAMS "release/bin/rttr.sh" DESTINATION "${RTTR_BINDIR}")
ELSE()
	message(FATAL_ERROR "${CMAKE_SYSTEM_NAME} not supported")
ENDIF()

# RTTR directory, but exclude language input files
install(DIRECTORY "RTTR" DESTINATION "${RTTR_DATADIR}"
		PATTERN "*.po" EXCLUDE
		PATTERN "*.pot" EXCLUDE
		PATTERN ".*" EXCLUDE
)

# Documentation. TODO: Avoid recursive entries?
IF (NOT WIN32)
	install(DIRECTORY "RTTR/texte/" DESTINATION "${RTTR_DOCDIR}"
			FILES_MATCHING PATTERN "*.txt"
	)
ENDIF()

# Placeholder for S2 installation
install(FILES "${RTTR_S2_PLACEHOLDER_PATH}" DESTINATION "${RTTR_GAMEDIR}")

################################################################################
# Custom targets
################################################################################

IF(NOT CMAKE_HOST_WIN32)
	# Special cleanup command "make mrproper"
	ADD_CUSTOM_TARGET(mrproper
					  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/build/cleanup.sh"
					  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
	)

	# Create nightly release
	ADD_CUSTOM_TARGET(create_nightly
					  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/release/create_release.sh" "nightly" "${CMAKE_SOURCE_DIR}"
					  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
	)

	# Create stable release
	ADD_CUSTOM_TARGET(create_stable
					  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/release/create_release.sh" "stable" "${CMAKE_SOURCE_DIR}"
					  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
	)
ENDIF()
