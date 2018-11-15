################################################################################
# MSVC
################################################################################
IF (MSVC)
  # Append msvcContribDir to CMAKE_PREFIX_PATH
  # Set CMAKE_LIBRARY_ARCHITECTURE and CONTRIB_DLL_DIR
	IF(CMAKE_SIZEOF_VOID_P EQUAL 4)
		SET(CMAKE_LIBRARY_ARCHITECTURE "x86" CACHE INTERNAL "")
	ELSE()
		SET(CMAKE_LIBRARY_ARCHITECTURE "x64" CACHE INTERNAL "")
	ENDIF()
	MESSAGE(STATUS "Building for MSVC: ${CMAKE_LIBRARY_ARCHITECTURE}")

	SET(msvcContribDir ${CMAKE_CURRENT_SOURCE_DIR}/full-contrib-msvc)
	IF(NOT EXISTS ${msvcContribDir} OR NOT IS_DIRECTORY ${msvcContribDir})
		MESSAGE(FATAL_ERROR "You have to extract contrib/full-contrib-msvc.rar to ${msvcContribDir} first!")
	ENDIF()
	SET(CONTRIB_DLL_DIR "${msvcContribDir}/bin/${CMAKE_LIBRARY_ARCHITECTURE}" CACHE INTERNAL "")
	IF(MSVC_VERSION GREATER 1800) # MSVC 2015
		list(APPEND CMAKE_PREFIX_PATH ${msvcContribDir}/VS2015)
	ENDIF()
	list(APPEND CMAKE_PREFIX_PATH ${msvcContribDir})
	list(APPEND CMAKE_PROGRAM_PATH ${msvcContribDir}/buildTools ${CONTRIB_DLL_DIR})
	set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} PARENT_SCOPE)
	set(CMAKE_PROGRAM_PATH ${CMAKE_PREFIX_PATH} PARENT_SCOPE)
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

# Cache so it can be used inside the functions below
set(RTTR_CONTRIB_DIR ${CMAKE_CURRENT_LIST_DIR} CACHE INTERNAL "")

function(enable_boost_backport)
  find_package(Boost 1.55 QUIET)
	set(tmpBoostVersion "${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}")
	IF(tmpBoostVersion VERSION_LESS 1.56)
		IF(MSVC AND MSVC_VERSION EQUAL 1800)
			# See https://svn.boost.org/trac/boost/ticket/9332
			MESSAGE(FATAL_ERROR "Boost 1.55 contains a bug so that it does not work with MSVC 2013. Use a newer boost version or a different Visual Studio")
		ENDIF()
		MESSAGE(STATUS "Boost version smaller than 1.56 detected. Using backport 1.55-1.56")
		INCLUDE_DIRECTORIES(${RTTR_CONTRIB_DIR}/backport/boost_1.55-1.56)
	ENDIF()
	IF(tmpBoostVersion VERSION_LESS 1.58)
		MESSAGE(STATUS "Boost version smaller than 1.58 detected. Using backport 1.56-1.58")
		INCLUDE_DIRECTORIES(${RTTR_CONTRIB_DIR}/backport/boost_1.56-1.58)
	ELSE()
		MESSAGE(STATUS "Boost ${Boost_VERSION} detected. No backport required")
	ENDIF()
endfunction()

function(add_interface_library name)
	if(CMAKE_VERSION VERSION_LESS 3.0)
			add_library(${name} STATIC ${RTTR_CONTRIB_DIR}/dummy.cpp)
	else()
			add_library(${name} INTERFACE)
	endif()
endfunction()

################################################################################
# Bzip2 sources
################################################################################

if(WIN32)
	set(bzip2ContribDir ${RTTR_CONTRIB_DIR}/bzip2-1.0.6)
	IF(IS_DIRECTORY ${bzip2ContribDir})
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

add_subdirectory("glad")
add_interface_library(kaguya)
target_include_directories(kaguya SYSTEM INTERFACE kaguya/include)
