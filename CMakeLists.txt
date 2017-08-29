CMAKE_MINIMUM_REQUIRED(VERSION 2.8.9)
# CMake 2.8.9 required for POSITION_INDEPENDENT_CODE target property
CMAKE_POLICY(SET CMP0003 NEW)
CMAKE_POLICY(SET CMP0015 NEW)
IF(NOT CMAKE_VERSION VERSION_LESS 3.0)
	# RPath enabled by default. Rpath only supported in CMake >2.8.12
	CMAKE_POLICY(SET CMP0042 OLD)
ENDIF()
IF(NOT CMAKE_VERSION VERSION_LESS 3.1)
	CMAKE_POLICY(SET CMP0053 NEW)
ENDIF()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules" "${CMAKE_SOURCE_DIR}/libsiedler2/cmake/Modules")
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/contrib")

INCLUDE(CMakeMacroSetCCache)
INCLUDE(CMakeMacroAddFlags)
INCLUDE(CMakeMacroForceAddFlags)
INCLUDE(CMakeMacroCorrectLib)
INCLUDE(CMakeMacroEnsureOutOfSourceBuild)
INCLUDE(CMakeDetectOsXArchs)

################################################################################

PROJECT(s25client)


################################################################################

IF (DEFINED CMAKE_TOOLCHAIN_FILE)
	MESSAGE(STATUS "Used Toolchain definition file '${CMAKE_TOOLCHAIN_FILE}'")
ENDIF()

################################################################################

STRING(TOLOWER ${CMAKE_SYSTEM_NAME} PLATFORM_NAME)
SET(PLATFORM_NAME ${PLATFORM_NAME} CACHE INTERNAL "Do not change")

# Set PLATFORM_ARCH
IF(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	IF ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
		SET(PLATFORM_ARCH "x86_64")
	ELSE ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
		SET(PLATFORM_ARCH "i386")
	ENDIF ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
ELSE()
	IF("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "i386" OR "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "i686")
		SET(PLATFORM_ARCH "i386")
	ELSEIF("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64" OR "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "amd64" OR "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "AMD64")
		SET(PLATFORM_ARCH "x86_64")
	ELSEIF(("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "universal" OR "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "") AND ${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
		SET(PLATFORM_ARCH "universal")
	ELSE()
		MESSAGE(FATAL_ERROR "Unknown processor arch: ${CMAKE_SYSTEM_PROCESSOR}")
	ENDIF()
ENDIF()
# Fill cache for post-processing tools
SET(PLATFORM_ARCH ${PLATFORM_ARCH} CACHE INTERNAL "Do not change")

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
# Default boost libs are static on self-build msvc versions and dynamic in the linux package repos
IF(MSVC)
	SET(RTTR_USE_STATIC_BOOST_DEFAULT ON)
ELSE()
	SET(RTTR_USE_STATIC_BOOST_DEFAULT OFF)
ENDIF()
	
OPTION(RTTR_USE_STATIC_BOOST "Build with static BOOST libraries instead of dynamic" ${RTTR_USE_STATIC_BOOST_DEFAULT})
IF((NOT "${Boost_USE_STATIC_LIBS}" AND ${RTTR_USE_STATIC_BOOST}) OR
   ("${Boost_USE_STATIC_LIBS}" AND NOT ${RTTR_USE_STATIC_BOOST}))
    set(Boost_USE_STATIC_LIBS ${RTTR_USE_STATIC_BOOST})
    unset(Boost_INCLUDE_DIR CACHE)
    unset(Boost_LIBRARY_DIRS CACHE)
ENDIF()
if(Boost_USE_STATIC_LIBS)
   message(STATUS "Linking against boost static libraries")
else(Boost_USE_STATIC_LIBS)
   message(STATUS "Linking against boost dynamic libraries")
endif(Boost_USE_STATIC_LIBS)

# For some reason boost needs sometimes 1.xy and sometimes 1.xy.0 to detect folder structures like boost_1_55_0
SET(TMP_VERSIONS "1.64" "1.63" "1.62" "1.61" "1.60" "1.59" "1.58" "1.57" "1.56" "1.55")
foreach(I_VERSION ${TMP_VERSIONS})
	list(APPEND Boost_ADDITIONAL_VERSIONS "${I_VERSION}.0" "${I_VERSION}")
endforeach()
UNSET(TMP_VERSIONS)

IF (MSVC)
	SET(RTTR_CONTRIB_DIR "${CMAKE_SOURCE_DIR}/contrib/full-contrib-msvc" CACHE PATH "Path to base dir from contrib package")
	IF(NOT EXISTS ${RTTR_CONTRIB_DIR} OR NOT IS_DIRECTORY ${RTTR_CONTRIB_DIR})
		MESSAGE(FATAL_ERROR "You have to extract contrib/full-contrib-msvc.rar to ${RTTR_CONTRIB_DIR} first!")
	ENDIF()
	IF(MSVC_VERSION GREATER 1800) # MSVC 2015
		list(APPEND CMAKE_PREFIX_PATH "${RTTR_CONTRIB_DIR}/VS2015")
	ENDIF()
	list(APPEND CMAKE_PREFIX_PATH ${RTTR_CONTRIB_DIR} ${RTTR_CONTRIB_DIR}/buildTools)

	IF("${PLATFORM_ARCH}" STREQUAL "i386")
		SET(CMAKE_LIBRARY_ARCHITECTURE "x86" CACHE STRING "")
	ELSE()
		SET(CMAKE_LIBRARY_ARCHITECTURE "x64" CACHE STRING "")
	ENDIF()
	MESSAGE(STATUS "Building for MSVC: ${CMAKE_LIBRARY_ARCHITECTURE}")
ENDIF()

IF(EXISTS "${CMAKE_SOURCE_DIR}/contrib/boost" AND IS_DIRECTORY "${CMAKE_SOURCE_DIR}/contrib/boost")
	SET(BOOST_ROOT ${CMAKE_SOURCE_DIR}/contrib/boost CACHE PATH "Path to find boost at")
ENDIF()

FIND_PACKAGE(Boost 1.55.0)
IF(NOT Boost_FOUND)
	MESSAGE(FATAL_ERROR "You have to install boost into contrib/boost or set (as CMake or environment variable) "
    "BOOST_ROOT (currently: '${BOOST_ROOT}', Environment: '$ENV{BOOST_ROOT}'), "
    "BOOST_INCLUDEDIR (currently: '${BOOST_INCLUDEDIR}', Environment: '$ENV{BOOST_INCLUDEDIR}'), "
    "since cmake was unable to find boost!")
ELSEIF("${BOOST_LIBRARYDIR}" STREQUAL "")
	SET(BOOST_LIBRARYDIR "${Boost_INCLUDE_DIR}/stage/lib/${CMAKE_LIBRARY_ARCHITECTURE}")
	MESSAGE(STATUS "Set boost library directory hint to: ${BOOST_LIBRARY_DIR}")
ENDIF()

SET(TMP_BOOST_VERSION "${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}")
IF(TMP_BOOST_VERSION VERSION_LESS 1.56)
	IF(MSVC AND MSVC_VERSION EQUAL 1800)
		# See https://svn.boost.org/trac/boost/ticket/9332
		MESSAGE(FATAL_ERROR "Boost 1.55 contains a bug so that it does not work with MSVC 2013. Use a newer boost version or a different Visual Studio")
	ENDIF()
	MESSAGE(STATUS "Boost version smaller than 1.56 detected. Using backport 1.55-1.56")
	INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/contrib/backport/boost_1.55-1.56")
ENDIF()
IF(TMP_BOOST_VERSION VERSION_LESS 1.58)
	MESSAGE(STATUS "Boost version smaller than 1.58 detected. Using backport 1.56-1.58")
	INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/contrib/backport/boost_1.56-1.58")
ELSE()
	MESSAGE(STATUS "Boost ${Boost_VERSION} detected. No backport required")
ENDIF()
UNSET(TMP_BOOST_VERSION)

find_package(ClangFormat 5.0.0)

################################################################################
# Set paths
# All paths are relative to the install-prefix.
# Example: BINDIR="bin", DATADIR="data" executed in "/foo/bar/bin/rttr.exe" -> FULL_DATADIR=/foo/bar/data
IF(NOT "${RTTR_PREFIX}" STREQUAL "")
	MESSAGE(FATAL_ERROR "Using RTTR_PREFIX was removed and the meaning of the paths have changed. Clear RTTR_PREFIX to disable this warning")
ENDIF()

IF ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
	# Windows uses one directory only
	SET(RTTR_BINDIR "." CACHE INTERNAL "")
	SET(RTTR_DATADIR "." CACHE INTERNAL "")
	SET(RTTR_GAMEDIR "." CACHE INTERNAL "")
	SET(RTTR_LIBDIR "." CACHE INTERNAL "")
	SET(RTTR_DOCDIR "." CACHE INTERNAL "")
	SET(RTTR_DRIVERDIR "./driver" CACHE INTERNAL "")
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
# Ensure relative paths
foreach(curDirVar RTTR_BINDIR RTTR_DATADIR RTTR_GAMEDIR RTTR_LIBDIR RTTR_DOCDIR RTTR_DRIVERDIR)
	if(IS_ABSOLUTE ${${curDirVar}})
		message(FATAL_ERROR "All RTTR_*DIR variables must be relative paths. ${curDirVar} is absolute: ${${curDirVar}}")
	endif()
endforeach()

SET(RTTR_SRCDIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "Path to source files. Used for some shell scripts")
set(RTTR_S2_PLACEHOLDER_PATH "${CMAKE_BINARY_DIR}/${RTTR_GAMEDIR}/put your S2-Installation in here")

# ensure out of source build
ENSURE_OUT_OF_SOURCE_BUILD("${PROJECT_NAME} requires an out of source build. Please go to the build directory and run './cmake.sh [options]' there.")

################################################################################
# Definitions and flags

INCLUDE(CheckCXXCompilerFlag)
INCLUDE(CheckCCompilerFlag)
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
	ADD_DEFINITIONS(/W3)
	ADD_DEFINITIONS(/MP) # parallel compilation
	# Signed/Unsigned operations
	ADD_DEFINITIONS(/w34389)
	IF(RTTR_ENABLE_WERROR)
		ADD_DEFINITIONS(/WX) # warning = error
	ENDIF()
	# Link time code generation (Required for bzlib) which also disables incremental linking
	ADD_FLAGS(CMAKE_EXE_LINKER_FLAGS /LTCG)
	FORCE_ADD_FLAGS(CMAKE_EXE_LINKER_FLAGS_DEBUG /INCREMENTAL:NO)
	FORCE_ADD_FLAGS(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO /INCREMENTAL:NO)
ELSE (MSVC)
	CheckAndAddFlag("-Wall")
	IF(RTTR_ENABLE_WERROR)
		CheckAndAddFlag("-Werror")
	ENDIF()
ENDIF (MSVC)
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
ENDIF(COMPILER_IN_CXX11_MODE)

# Variadic macros are part of C99 but supported by all big compilers in C++03
CheckAndAddFlag("-Wno-variadic-macros")
CheckAndAddFlag("-Wno-c99-extensions")
ADD_DEFINITIONS(-DBOOST_PP_VARIADICS=1)

# VS does not support coverage analysis
if(NOT MSVC)
	option(RTTR_ENABLE_COVERAGE "Generate coverage build" OFF)
	if(RTTR_ENABLE_COVERAGE)
		if(NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
			message(FATAL_ERROR "Coverage requires a debug build or you get false positives")
		endif()
		# Note: "--coverage" instead of "-coverage" makes this work with ccache
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 --coverage")
		if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
			# Inlining makes the coverage statistic much harder to read and may lead to lots of partials
			# However expect a slowdown!
			message(STATUS "Enabled no-inline for better coverage statistics")
			set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-inline -fno-inline-small-functions -fno-default-inline")
		else()
			message(STATUS "Using default inlining which probably influences coverage statistics")
		endif()
	endif()
endif()

IF ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
	# Enforce unicode
	ADD_DEFINITIONS(-DUNICODE)
	ADD_DEFINITIONS(-D_UNICODE)
ENDIF()

INCLUDE(CMakeDetectCxxFeatures)
IF (NOT CXX_HAVE_OVERRIDE)
	ADD_DEFINITIONS(-Doverride=)
ENDIF()

INCLUDE(CheckIncludeFile)
CHECK_INCLUDE_FILE(valgrind/memcheck.h HAVE_MEMCHECK_H)
IF(HAVE_MEMCHECK_H)
    ADD_DEFINITIONS(-DHAVE_MEMCHECK_H)
ENDIF()

if(CMAKE_CXX_COMPILER_ID MATCHES Clang)
    try_compile(CHECK_CLANG_INLINE "${CMAKE_CURRENT_BINARY_DIR}/cxx" "${CMAKE_SOURCE_DIR}/cmake/checkclang.cpp")
    if(NOT ${CHECK_CLANG_INLINE})
        add_definitions("-D__extern_always_inline=extern __always_inline __attribute__ ((__gnu_inline__))")
    endif()

    # Some workaround flags for execssive warnings in clang
    CheckAndAddFlag(-Wno-unused-local-typedefs)
endif()

################################################################################
# Create symlinks for easier debugging

MACRO(symlinkFolder dst symLinkPath)
	IF(NOT EXISTS "${symLinkPath}")
		IF(EXISTS "${dst}")
			set(symlink_failed)
			message(STATUS "Creating symlink: '${symLinkPath}' -> '${dst}'")
			execute_process(COMMAND ln -vs "${dst}" "${symLinkPath}" RESULT_VARIABLE symlink_failed)
			if(symlink_failed)
				message(FATAL_ERROR "Creating symlink failed")
			endif(symlink_failed)
		ELSE()
			message(WARNING "Directory '${dst}' missing!")
		ENDIF()
	ENDIF()
ENDMACRO(symlinkFolder)

IF(NOT ${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")
	file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/${RTTR_DATADIR}")
	symlinkFolder("${CMAKE_SOURCE_DIR}/RTTR" "${CMAKE_BINARY_DIR}/${RTTR_DATADIR}/RTTR")
	file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/${RTTR_LIBDIR}/RTTR")
	if(NOT EXISTS "${CMAKE_SOURCE_DIR}/S2")
		file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/S2")
	endif()
	symlinkFolder("${CMAKE_SOURCE_DIR}/S2" "${CMAKE_BINARY_DIR}/${RTTR_GAMEDIR}")
ELSE()
	message(STATUS "Host system is ${CMAKE_HOST_SYSTEM_NAME}. Will use copies instead of symlinks")
	file(COPY "${CMAKE_SOURCE_DIR}/RTTR" DESTINATION "${CMAKE_BINARY_DIR}"
		 PATTERN "*.po" EXCLUDE
		 PATTERN "*.pot" EXCLUDE
		 PATTERN ".*" EXCLUDE
	)
ENDIF()

################################################################################
# Configure files

CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/build_paths.h.cmake"
			   build_paths.h @ONLY)
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/build_version_defines.h.cmake"
			   build_version_defines.h @ONLY)
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/release/prepareRelease.sh.cmake"
		   ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/prepareRelease.sh @ONLY)
file(COPY ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/prepareRelease.sh
	DESTINATION "."
	FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
	GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
IF (NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
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
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR})

enable_testing()

################################################################################
# Add version generater first

ADD_SUBDIRECTORY(version)

ADD_CUSTOM_TARGET(updateversion ALL
				  COMMAND "$<TARGET_FILE:version>" "${CMAKE_SOURCE_DIR}"
				  DEPENDS version
				  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)
	
################################################################################
# Then add all other dependencies

FILE(GLOB COMMON_SRC ${PROJECT_SOURCE_DIR}/commonSrc/*.cpp ${PROJECT_SOURCE_DIR}/commonSrc/*.h)

ADD_SUBDIRECTORY(driver)
ADD_SUBDIRECTORY(libendian)
ADD_SUBDIRECTORY(mygettext)
ADD_SUBDIRECTORY(libsiedler2)
ADD_SUBDIRECTORY(libutil)
# Build lobby client
SET(LOBBY_C TRUE)
ADD_SUBDIRECTORY(liblobby)
ADD_SUBDIRECTORY(s-c)
ADD_SUBDIRECTORY(s25update)
ADD_SUBDIRECTORY(src)

if(ClangFormat_FOUND)
    add_ClangFormat_folder(${CMAKE_CURRENT_SOURCE_DIR}/driver true)
    add_ClangFormat_folder(${CMAKE_CURRENT_SOURCE_DIR}/src true)
    add_ClangFormat_target("file")
endif()

################################################################################
# Install

# Start script
IF ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
	install(PROGRAMS "release/bin/rttr.bat" DESTINATION "${RTTR_BINDIR}")
ELSEIF("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" OR "${CMAKE_SYSTEM_NAME}" STREQUAL "FreeBSD")
	install(PROGRAMS "release/bin/rttr.sh" DESTINATION "${RTTR_BINDIR}")
ELSEIF(NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
	message(FATAL_ERROR "${CMAKE_SYSTEM_NAME} not supported")
ENDIF()

# RTTR directory, but exclude language input files
install(DIRECTORY "RTTR" DESTINATION "${RTTR_DATADIR}"
		PATTERN "*.po" EXCLUDE
		PATTERN "*.pot" EXCLUDE
		PATTERN ".*" EXCLUDE
)

# Documentation. TODO: Avoid recursive entries?
IF (NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
	install(DIRECTORY "RTTR/texte/" DESTINATION "${RTTR_DOCDIR}"
			FILES_MATCHING PATTERN "*.txt"
	)
ENDIF()

# Placeholder for S2 installation
install(FILES "${RTTR_S2_PLACEHOLDER_PATH}" DESTINATION "${RTTR_GAMEDIR}")

IF(NOT ${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")
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
