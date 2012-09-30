################################################################################
### $Id: CMakeLists.txt 8361 2012-09-30 16:52:07Z FloSoft $
################################################################################

CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0)
CMAKE_POLICY(SET CMP0003 OLD)

IF(${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} GREATER 7 AND ${CMAKE_PATCH_VERSION} GREATER 0)
	CMAKE_POLICY(SET CMP0015 OLD)
ENDIF(${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} GREATER 7 AND ${CMAKE_PATCH_VERSION} GREATER 0)

SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)

INCLUDE(CMakeMacroSetCCache)
INCLUDE(CMakeMacroAddFlags)
INCLUDE(CMakeMacroForceAddFlags)
INCLUDE(CMakeMacroCorrectLib)
INCLUDE(CMakeMacroEnsureOutOfSourceBuild)

################################################################################

INCLUDE(cmake/crosscompile.cmake)

PROJECT(s25client)

################################################################################

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

# avoid lhs-path bug in windows
IF ("${COMPILEFOR}" STREQUAL "windows")
	SET(CMAKE_INSTALL_PREFIX "" CACHE STRING "" FORCE)
ENDIF ("${COMPILEFOR}" STREQUAL "windows")

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

CHECK_CXX_COMPILER_FLAG("-Wall -Werror -pedantic -Wno-long-long -fno-strict-aliasing" WERROR_FLAGS)
IF (WERROR_FLAGS)
	IF (NOT "${COMPILEFOR}" STREQUAL "windows" )
		ADD_DEFINITIONS(-Wall -pedantic -fno-strict-aliasing)
	ELSE (NOT "${COMPILEFOR}" STREQUAL "windows" )
		ADD_DEFINITIONS(-Wall -pedantic -Wno-long-long) # win32 cross build with -Werror does not work
	ENDIF (NOT "${COMPILEFOR}" STREQUAL "windows" )
ENDIF (WERROR_FLAGS)

# avoid lhs-path bug in windows
IF ("${COMPILEFOR}" STREQUAL "windows")
	SET(BINDIR "." CACHE STRING "" FORCE)
	SET(DATADIR "." CACHE STRING "" FORCE)
	SET(LIBDIR "." CACHE STRING "" FORCE)
	SET(DRIVERDIR "./driver" CACHE STRING "" FORCE)
	ADD_DEFINITIONS(-D_WIN32_WINNT=0x0501)
	ADD_DEFINITIONS(-D__CYGWIN__)
ENDIF ("${COMPILEFOR}" STREQUAL "windows")

# add build directory to compiler search path
FORCE_ADD_FLAGS(CMAKE_C_FLAGS -I${CMAKE_BINARY_DIR})
FORCE_ADD_FLAGS(CMAKE_CXX_FLAGS -I${CMAKE_BINARY_DIR})

################################################################################

CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/build_paths.h.cmake"
			   build_paths.h)
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/build_version.h.cmake"
			   build_version.h)
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/build/preinstall.sh.cmake"
			   preinstall.sh @ONLY)
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/build/postinstall.sh.cmake"
			   postinstall.sh @ONLY)

################################################################################

ADD_SUBDIRECTORY(version)

ADD_CUSTOM_TARGET(updateversion ALL
				  COMMAND "${CMAKE_BINARY_DIR}/version/src/version" "${CMAKE_SOURCE_DIR}"
				  DEPENDS version
				  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)

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
  
# postinstall script
INSTALL(SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/cmake/postinstall.cmake")

INSTALL(CODE "MESSAGE(STATUS \"Installation is now complete\")")

################################################################################
