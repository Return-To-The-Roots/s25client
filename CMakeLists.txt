
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.7)
CMAKE_POLICY(SET CMP0003 NEW)
CMAKE_POLICY(SET CMP0015 NEW)
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

OPTION(USE_STATIC_BOOST "Build with static BOOST libraries instead of dynamic" YES)
IF(NOT ${Boost_USE_STATIC_LIBS} EQUAL ${USE_STATIC_BOOST})
    set(Boost_USE_STATIC_LIBS ${USE_STATIC_BOOST})
    unset(Boost_INCLUDE_DIR CACHE)
    unset(Boost_LIBRARY_DIRS CACHE)
ENDIF()
if(Boost_USE_STATIC_LIBS)
   message(STATUS "Linking against boost static libraries")
else(Boost_USE_STATIC_LIBS)
   message(STATUS "Linking against boost dynamic libraries")
endif(Boost_USE_STATIC_LIBS)

SET(Boost_ADDITIONAL_VERSIONS "1.55" "1.56" "1.57" "1.58" "1.59")

IF (MSVC)
	IF(NOT EXISTS "${CMAKE_SOURCE_DIR}/contrib/full-contrib-msvc2010" OR NOT IS_DIRECTORY "${CMAKE_SOURCE_DIR}/contrib/full-contrib-msvc2010")
		MESSAGE(FATAL_ERROR "You have to extract contrib/full-contrib-msvc2010.rar to contrib/full-contrib-msvc2010 first!")
	ENDIF()
	SET(CONTRIB_DIR "${CMAKE_SOURCE_DIR}/contrib/full-contrib-msvc2010" CACHE PATH "Path to base dir from contrib package")
	IF(MSVC_VERSION GREATER 1800) # MSVC 2015
		SET(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${CONTRIB_DIR}/VS2015")
	ENDIF()
	SET(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${CONTRIB_DIR})		
ENDIF()

IF(EXISTS "${CMAKE_SOURCE_DIR}/contrib/boost" AND IS_DIRECTORY "${CMAKE_SOURCE_DIR}/contrib/boost")
	SET(BOOST_ROOT ${CMAKE_SOURCE_DIR}/contrib/boost CACHE PATH "Path to find boost at")
ENDIF()

FIND_PACKAGE(Boost 1.55.0)
IF(NOT Boost_FOUND)
	MESSAGE(FATAL_ERROR "You have to install boost into contrib/boost or set BOOST_ROOT environment variable (currently: ${BOOST_ROOT}), since cmake was unable to find boost!")
ENDIF()

SET(TMP_BOOST_VERSION "${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}")
IF(TMP_BOOST_VERSION VERSION_LESS 1.56)
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

IF ("${PREFIX}" STREQUAL "")
    IF ("${CMAKE_INSTALL_PREFIX}" STREQUAL "")
        SET(PREFIX .)
    ELSE()
        SET(PREFIX ${CMAKE_INSTALL_PREFIX})
    ENDIF()
ENDIF()

SET(PREFIX ${PREFIX} CACHE STRING "Path prefix")
SET(BINDIR ${BINDIR} CACHE STRING "Directory for the binaries")
SET(DATADIR ${DATADIR} CACHE STRING "Data directory")
SET(LIBDIR ${LIBDIR} CACHE STRING "Directory for shared binaries")
SET(DRIVERDIR ${DRIVERDIR} CACHE STRING "Directory for driver objects")

IF (NOT "${PREFIX}" STREQUAL "" AND "${BINDIR}" STREQUAL "")
	SET(BINDIR ${PREFIX}/bin CACHE STRING "Directory for the binaries" FORCE)
	MESSAGE(STATUS "Set BINDIR to ${BINDIR}")
ENDIF (NOT "${PREFIX}" STREQUAL "" AND "${BINDIR}" STREQUAL "")

IF (NOT "${PREFIX}" STREQUAL "" AND "${DATADIR}" STREQUAL "")
	SET(DATADIR ${PREFIX}/share/s25rttr CACHE STRING "Data directory" FORCE)
	MESSAGE(STATUS "Set DATADIR to ${DATADIR}")
ENDIF (NOT "${PREFIX}" STREQUAL "" AND "${DATADIR}" STREQUAL "")

IF (NOT "${DATADIR}" STREQUAL "" AND "${LIBDIR}" STREQUAL "")
	SET(LIBDIR ${DATADIR} CACHE STRING "Directory for shared binaries" FORCE)
	MESSAGE(STATUS "Set LIBDIR to ${LIBDIR}")
ENDIF (NOT "${DATADIR}" STREQUAL "" AND "${LIBDIR}" STREQUAL "")

IF (NOT "${LIBDIR}" STREQUAL "" AND "${DRIVERDIR}" STREQUAL "")
	SET(DRIVERDIR "${LIBDIR}/driver" CACHE STRING "Directory for driver objects" FORCE)
	MESSAGE(STATUS "Set DRIVERDIR to ${DRIVERDIR}")
ENDIF (NOT "${LIBDIR}" STREQUAL "" AND "${DRIVERDIR}" STREQUAL "")

IF(NOT "$ENV{CMAKE_INSTALL_PREFIX}" STREQUAL "")
	SET(ENV{DESTDIR} "$ENV{CMAKE_INSTALL_PREFIX}" CACHE STRING "" FORCE)
	MESSAGE(STATUS "Set DESTDIR to $ENV{DESTDIR}")
ENDIF(NOT "$ENV{CMAKE_INSTALL_PREFIX}" STREQUAL "")

IF(NOT "$ENV{DESTDIR}" STREQUAL "")
	SET(ENV{DESTDIR} "$ENV{DESTDIR}/" CACHE STRING "" FORCE)
	SET(CMAKE_INSTALL_PREFIX "" CACHE STRING "" FORCE)
	MESSAGE(STATUS "Corrected DESTDIR to $ENV{DESTDIR}")
ENDIF(NOT "$ENV{DESTDIR}" STREQUAL "")

# ensure out of source build
ENSURE_OUT_OF_SOURCE_BUILD("${PROJECT_NAME} requires an out of source build. Please go to the build directory and run './cmake.sh [options]' there.")

# Build lobby client
SET (LOBBY_C TRUE)

################################################################################

INCLUDE(CheckCXXCompilerFlag)
MACRO(CheckAndAddFlag flag)
	CHECK_CXX_COMPILER_FLAG(${flag} FLAG_${flag}_SUPPORTED)
	IF (FLAG_${flag}_SUPPORTED)
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
CheckAndAddFlag("-fno-strict-aliasing")

# avoid lhs-path bug in windows
IF ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
	SET(BINDIR "." CACHE STRING "" FORCE)
	SET(DATADIR "." CACHE STRING "" FORCE)
	SET(LIBDIR "." CACHE STRING "" FORCE)
	SET(DRIVERDIR "./driver" CACHE STRING "" FORCE)
	#ADD_DEFINITIONS(-D_WIN32_WINNT=0x0501)
ENDIF ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")

# add build directory to compiler search path
FORCE_ADD_FLAGS(CMAKE_C_FLAGS -I${CMAKE_BINARY_DIR})
FORCE_ADD_FLAGS(CMAKE_CXX_FLAGS -I${CMAKE_BINARY_DIR})

INCLUDE(CMakeDetectCxxFeatures)
IF (CXX_HAVE_CONSTEXPR)
	ADD_DEFINITIONS(-DCONSTEXPR=constexpr)
ELSE()
	ADD_DEFINITIONS(-DCONSTEXPR=const)
ENDIF()
IF (NOT CXX_HAVE_OVERRIDE)
	ADD_DEFINITIONS(-Doverride=)
ENDIF()


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
