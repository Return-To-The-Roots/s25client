set(RTTR_CONTRIB_DIR ${CMAKE_SOURCE_DIR}/contrib)

list(APPEND CMAKE_PREFIX_PATH "${RTTR_CONTRIB_DIR}")

################################################################################
# MSVC
################################################################################
IF (MSVC)
    # Fill RTTR_CONTRIB_DIR_VS and append to CMAKE_PREFIX_PATH
    # Set CMAKE_LIBRARY_ARCHITECTURE and CONTRIB_DLL_DIR
	IF(CMAKE_SIZEOF_VOID_P EQUAL 4)
		SET(CMAKE_LIBRARY_ARCHITECTURE "x86" CACHE INTERNAL "")
	ELSE()
		SET(CMAKE_LIBRARY_ARCHITECTURE "x64" CACHE INTERNAL "")
	ENDIF()
	MESSAGE(STATUS "Building for MSVC: ${CMAKE_LIBRARY_ARCHITECTURE}")

	SET(RTTR_CONTRIB_DIR_VS "${RTTR_CONTRIB_DIR}/full-contrib-msvc" CACHE PATH "Path to base dir from contrib package")
	IF(NOT EXISTS ${RTTR_CONTRIB_DIR_VS} OR NOT IS_DIRECTORY ${RTTR_CONTRIB_DIR_VS})
		MESSAGE(FATAL_ERROR "You have to extract contrib/full-contrib-msvc.rar to ${RTTR_CONTRIB_DIR_VS} first!")
	ENDIF()
	SET(CONTRIB_DLL_DIR "${RTTR_CONTRIB_DIR_VS}/bin/${CMAKE_LIBRARY_ARCHITECTURE}")
	IF(MSVC_VERSION GREATER 1800) # MSVC 2015
		list(APPEND CMAKE_PREFIX_PATH "${RTTR_CONTRIB_DIR_VS}/VS2015")
	ENDIF()
	list(APPEND CMAKE_PREFIX_PATH ${RTTR_CONTRIB_DIR_VS})
	list(APPEND CMAKE_PROGRAM_PATH ${RTTR_CONTRIB_DIR_VS}/buildTools ${CONTRIB_DLL_DIR})
ENDIF()

################################################################################
# Boost
################################################################################

include(RttrBoostCfg)

IF(EXISTS "${RTTR_CONTRIB_DIR}/boost" AND IS_DIRECTORY "${RTTR_CONTRIB_DIR}/boost")
	SET(BOOST_ROOT ${RTTR_CONTRIB_DIR}/boost CACHE PATH "Path to find boost at")
ENDIF()

FIND_PACKAGE(Boost 1.55.0 QUIET)
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
