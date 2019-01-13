include(RttrTestingCfg)
option(RTTR_EXTERNAL_BUILD_TESTING "Enabe tests of external dependencies" OFF)
set(BUILD_TESTING ${RTTR_EXTERNAL_BUILD_TESTING})

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
	SET(_dllDir "${msvcContribDir}/bin/${CMAKE_LIBRARY_ARCHITECTURE}")
    # Add dlls to the gathered ones
	if(NOT EXISTS ${_dllDir}/libcurl.dll)
		message(WARNING "Folder with DLLs not found in ${_dllDir}. You may not be able to execute directly from VS")
	else()
		file(GLOB CONTRIB_DLLS ${_dllDir}/*.dll)
		include(GatherDll)
		gather_dll_add(${CONTRIB_DLLS})
	endif()
	list(APPEND CMAKE_PREFIX_PATH ${msvcContribDir})
	list(APPEND CMAKE_PROGRAM_PATH ${msvcContribDir}/buildTools ${_dllDir})
	set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} PARENT_SCOPE)
	set(CMAKE_PROGRAM_PATH ${CMAKE_PROGRAM_PATH} PARENT_SCOPE)
ENDIF()

################################################################################
# Bzip2 sources
################################################################################

if(WIN32)
	set(bzip2ContribDir ${CMAKE_CURRENT_LIST_DIR}/bzip2-1.0.6)
	IF(IS_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/${bzip2ContribDir})
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

add_subdirectory(glad)
add_library(kaguya INTERFACE)
target_include_directories(kaguya SYSTEM INTERFACE kaguya/include)
add_subdirectory(libendian)
set(RTTR_BUILD_LOBBY_CLIENT ON)
add_subdirectory(liblobby)
add_subdirectory(libsiedler2)
add_subdirectory(libutil)
add_subdirectory(mygettext)
add_subdirectory(s-c)
add_subdirectory(s25edit)
add_subdirectory(s25update)
add_library(turtle INTERFACE)
target_include_directories(turtle SYSTEM INTERFACE turtle/include)
target_compile_definitions(turtle INTERFACE MOCK_NO_AUTO_PTR)
