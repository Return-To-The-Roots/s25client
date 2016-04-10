CMAKE_MINIMUM_REQUIRED(VERSION 2.8.7)
CMAKE_POLICY(SET CMP0003 NEW)
CMAKE_POLICY(SET CMP0015 NEW)
IF(NOT CMAKE_VERSION VERSION_LESS 3.0)
	# RPath enabled by default. Rpath only supported in CMake >2.8.12
	CMAKE_POLICY(SET CMP0042 OLD)
ENDIF()
IF(NOT CMAKE_VERSION VERSION_LESS 3.1)
	CMAKE_POLICY(SET CMP0053 NEW)
ENDIF()

SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)
SET(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${CMAKE_SOURCE_DIR}/contrib)

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

STRING(TOLOWER ${CMAKE_SYSTEM_NAME} PLATFORM_NAME)
SET(PLATFORM_NAME ${PLATFORM_NAME} CACHE STRING "Do not change" FORCE)

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
SET(PLATFORM_ARCH ${PLATFORM_ARCH} CACHE STRING "Do not change" FORCE)

MARK_AS_ADVANCED(PLATFORM_NAME PLATFORM_ARCH)

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
SET(Boost_ADDITIONAL_VERSIONS "1.59.0" "1.59" "1.58.0" "1.58" "1.57.0" "1.57" "1.56.0" "1.56" "1.55.0" "1.55")

IF (MSVC)
	SET(RTTR_CONTRIB_DIR "${CMAKE_SOURCE_DIR}/contrib/full-contrib-msvc" CACHE PATH "Path to base dir from contrib package")
	IF(NOT EXISTS ${RTTR_CONTRIB_DIR} OR NOT IS_DIRECTORY ${RTTR_CONTRIB_DIR})
		MESSAGE(FATAL_ERROR "You have to extract contrib/full-contrib-msvc.rar to ${RTTR_CONTRIB_DIR} first!")
	ENDIF()
	IF(MSVC_VERSION GREATER 1800) # MSVC 2015
		SET(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${RTTR_CONTRIB_DIR}/VS2015")
	ENDIF()
	SET(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${RTTR_CONTRIB_DIR} ${RTTR_CONTRIB_DIR}/buildTools)		

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
	MESSAGE(FATAL_ERROR "You have to install boost into contrib/boost or set BOOST_ROOT environment variable (currently: ${BOOST_ROOT}), since cmake was unable to find boost!")
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

# avoid lhs-path bug in windows
IF ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
	SET(CMAKE_INSTALL_PREFIX "" CACHE STRING "" FORCE)
ENDIF ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")

################################################################################

IF ("${RTTR_PREFIX}" STREQUAL "")
    IF ("${CMAKE_INSTALL_PREFIX}" STREQUAL "")
        SET(RTTR_PREFIX .)
    ELSE()
        SET(RTTR_PREFIX ${CMAKE_INSTALL_PREFIX})
    ENDIF()
ENDIF()

SET(RTTR_PREFIX ${RTTR_PREFIX} CACHE STRING "Path prefix")
SET(RTTR_BINDIR ${RTTR_BINDIR} CACHE STRING "Directory for the binaries")
SET(RTTR_DATADIR ${RTTR_DATADIR} CACHE STRING "Data directory")
SET(RTTR_LIBDIR ${RTTR_LIBDIR} CACHE STRING "Directory for shared binaries")
SET(RTTR_DRIVERDIR ${RTTR_DRIVERDIR} CACHE STRING "Directory for driver objects")

IF (NOT "${RTTR_PREFIX}" STREQUAL "" AND "${RTTR_BINDIR}" STREQUAL "")
	SET(RTTR_BINDIR ${RTTR_PREFIX}/bin CACHE STRING "Directory for the binaries" FORCE)
	MESSAGE(STATUS "Set RTTR_BINDIR to ${RTTR_BINDIR}")
ENDIF ()

IF (NOT "${RTTR_PREFIX}" STREQUAL "" AND "${RTTR_DATADIR}" STREQUAL "")
	SET(RTTR_DATADIR ${RTTR_PREFIX}/share/s25rttr CACHE STRING "Data directory" FORCE)
	MESSAGE(STATUS "Set RTTR_DATADIR to ${RTTR_DATADIR}")
ENDIF()

IF (NOT "${RTTR_DATADIR}" STREQUAL "" AND "${RTTR_LIBDIR}" STREQUAL "")
	SET(RTTR_LIBDIR ${RTTR_DATADIR} CACHE STRING "Directory for shared binaries" FORCE)
	MESSAGE(STATUS "Set RTTR_LIBDIR to ${RTTR_LIBDIR}")
ENDIF()

IF (NOT "${RTTR_LIBDIR}" STREQUAL "" AND "${RTTR_DRIVERDIR}" STREQUAL "")
	SET(RTTR_DRIVERDIR "${RTTR_LIBDIR}/driver" CACHE STRING "Directory for driver objects" FORCE)
	MESSAGE(STATUS "Set RTTR_DRIVERDIR to ${RTTR_DRIVERDIR}")
ENDIF()

# Change DESTDIR if required. Note: This is a variable used by cmake and install, do not rename!
# -> https://cmake.org/cmake/help/v3.0/variable/CMAKE_INSTALL_PREFIX.html
IF(NOT "$ENV{CMAKE_INSTALL_PREFIX}" STREQUAL "")
	SET(ENV{DESTDIR} "$ENV{CMAKE_INSTALL_PREFIX}" CACHE STRING "" FORCE)
	MESSAGE(STATUS "Set DESTDIR to $ENV{DESTDIR}")
ENDIF()

IF(NOT "$ENV{DESTDIR}" STREQUAL "")
	SET(ENV{DESTDIR} "$ENV{DESTDIR}/" CACHE STRING "" FORCE)
	SET(CMAKE_INSTALL_PREFIX "" CACHE STRING "" FORCE)
	MESSAGE(STATUS "Corrected DESTDIR to $ENV{DESTDIR}")
ENDIF()

# ensure out of source build
ENSURE_OUT_OF_SOURCE_BUILD("${PROJECT_NAME} requires an out of source build. Please go to the build directory and run './cmake.sh [options]' there.")

# Build lobby client
SET (LOBBY_C TRUE)

################################################################################

INCLUDE(CheckCXXCompilerFlag)
MACRO(CheckAndAddFlag flag)
	SET(VarName ${flag})
	STRING(REPLACE "+" "X" VarName ${VarName})
	CHECK_CXX_COMPILER_FLAG(${flag} FLAG_${VarName}_SUPPORTED)
	IF (FLAG_${VarName}_SUPPORTED)
		ADD_DEFINITIONS(${flag})
	ENDIF ()
ENDMACRO()

IF (MSVC)
	CheckAndAddFlag("/W3")
ELSE (MSVC)
	CheckAndAddFlag("-Wall")
ENDIF (MSVC)
CheckAndAddFlag("-pedantic")
CheckAndAddFlag("-Wno-long-long")
CheckAndAddFlag("-Wno-deprecated-register")
CheckAndAddFlag("-Wno-unknown-pragmas")
CheckAndAddFlag("-Wno-c++11-extensions")
CheckAndAddFlag("-fno-strict-aliasing")
CheckAndAddFlag("-Qunused-arguments")

# Variadic macros are part of C99 but supported by all big compilers in C++03
CheckAndAddFlag("-Wno-variadic-macros")
CheckAndAddFlag("-Wno-c99-extensions")
ADD_DEFINITIONS(-DBOOST_PP_VARIADICS=1)

# avoid lhs-path bug in windows
IF ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
	SET(RTTR_BINDIR "." CACHE STRING "" FORCE)
	SET(RTTR_DATADIR "." CACHE STRING "" FORCE)
	SET(RTTR_LIBDIR "." CACHE STRING "" FORCE)
	SET(RTTR_DRIVERDIR "./driver" CACHE STRING "" FORCE)
	#ADD_DEFINITIONS(-D_WIN32_WINNT=0x0501)
ENDIF()

# add build directory to compiler search path
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})

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
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -D__extern_always_inline=\"extern __always_inline __attribute__ ((__gnu_inline__))\"" )
    endif()
endif()

################################################################################

CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/build_paths.h.cmake"
			   build_paths.h)
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/build_version.h.cmake"
			   build_version.h)
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/build_version_defines.h.cmake"
			   build_version_defines.h)
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/build/preinstall.sh.cmake"
			   preinstall.sh @ONLY)
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/build/postinstall.sh.cmake"
			   postinstall.sh @ONLY)

################################################################################
# Create symlinks

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
	symlinkFolder("${CMAKE_SOURCE_DIR}/RTTR" "${CMAKE_BINARY_DIR}/RTTR")
	
	# do not create debug-run symlinks on jenkins (too many levels of symbolic links "bug")
	IF("$ENV{JENKINS_URL}" STREQUAL "")
		symlinkFolder("${CMAKE_BINARY_DIR}" "${CMAKE_BINARY_DIR}/bin")
		symlinkFolder("${CMAKE_BINARY_DIR}" "${CMAKE_BINARY_DIR}/share")
		symlinkFolder("${CMAKE_BINARY_DIR}" "${CMAKE_BINARY_DIR}/s25rttr")
		symlinkFolder("${CMAKE_SOURCE_DIR}/S2" "${CMAKE_BINARY_DIR}/S2")
	ENDIF()
ELSE()
	message(STATUS "Host system is ${CMAKE_HOST_SYSTEM_NAME}. Will use copies instead of symlinks")
ENDIF()

################################################################################

ADD_SUBDIRECTORY(version)

ADD_CUSTOM_TARGET(updateversion ALL
				  COMMAND "$<TARGET_FILE:version>" "${CMAKE_SOURCE_DIR}"
				  DEPENDS version
				  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)
	
IF(NOT ${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")
	ADD_CUSTOM_TARGET(mrproper
					  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/build/cleanup.sh"
					  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
	)

	ADD_CUSTOM_TARGET(create_nightly
					  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/release/create_release.sh" "nightly" "${CMAKE_SOURCE_DIR}"
					  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
	)

	ADD_CUSTOM_TARGET(create_stable
					  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/release/create_release.sh" "stable" "${CMAKE_SOURCE_DIR}"
					  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
	)

	# preinstall script
	INSTALL(SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/cmake/preinstall.cmake")
ENDIF()

################################################################################

ADD_SUBDIRECTORY(driver)
ADD_SUBDIRECTORY(libendian)
ADD_SUBDIRECTORY(mygettext)
ADD_SUBDIRECTORY(libsiedler2)
ADD_SUBDIRECTORY(libutil)
ADD_SUBDIRECTORY(liblobby)
ADD_SUBDIRECTORY(s-c)
ADD_SUBDIRECTORY(s25update)
ADD_SUBDIRECTORY(src)

################################################################################

# postinstall script-bug workaround, including sub-install-scripts manually
INSTALL(CODE "INCLUDE(driver/cmake_install.cmake)")
INSTALL(CODE "INCLUDE(libsiedler2/cmake_install.cmake)")
INSTALL(CODE "INCLUDE(s-c/cmake_install.cmake)")
INSTALL(CODE "INCLUDE(s25update/cmake_install.cmake)")
INSTALL(CODE "INCLUDE(src/cmake_install.cmake)")

# and then dont let him install anything
INSTALL(CODE "SET(CMAKE_INSTALL_LOCAL_ONLY 1)")
  
IF(NOT ${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")
	# postinstall script
	INSTALL(SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/cmake/postinstall.cmake")
ENDIF()

INSTALL(CODE "MESSAGE(STATUS \"Installation is now complete\")")

################################################################################
