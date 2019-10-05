include(RttrTestingCfg)
option(RTTR_EXTERNAL_BUILD_TESTING "Enable tests of external dependencies" OFF)
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
		MESSAGE(FATAL_ERROR "You have to extract contrib/full-contrib-msvc.7z to ${msvcContribDir} first!")
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
# LUA
################################################################################

# Here we set hints to lua libraries used for official builds Normally those should be
# on the build server but we make them available here for convenience.
# As this will be appended to CMAKE_PREFIX_PATH one can use other version by setting
# that appropriately

set(_contrib_lua_path ${CMAKE_CURRENT_LIST_DIR}/lua)

if(MSVC)
	# Library is in contrib archive which is in prefix path.
	# We just need the includes (which we did not duplicate in the msvc-contrib)
	list(APPEND CMAKE_PREFIX_PATH ${_contrib_lua_path})
	set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} PARENT_SCOPE)
elseif(WIN32 OR CYGWIN)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(_contrib_lua_libpath ${_contrib_lua_path}/win64)
	elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
		set(_contrib_lua_libpath ${_contrib_lua_path}/win32)
	endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "i.86")
		set(_contrib_lua_libpath ${_contrib_lua_path}/lin32)
	elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "amd64|AMD64|x86_64")
		set(_contrib_lua_libpath ${_contrib_lua_path}/lin64)
	endif()
elseif(APPLE)
	set(_contrib_lua_libpath ${_contrib_lua_path}/mac)
endif()

# Set only if we have a library for this arch
if(_contrib_lua_libpath)
	if(NOT EXISTS ${_contrib_lua_path}/include/lua.h)
		message(WARNING "Could not find lua.h in contrib")
	endif()
	# For cross compilation we must set CMAKE_FIND_ROOT_PATH instead
	if(CMAKE_CROSSCOMPILING)
		list(APPEND CMAKE_FIND_ROOT_PATH ${_contrib_lua_path} ${_contrib_lua_libpath})
		set(CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH} PARENT_SCOPE)
	else()
		list(APPEND CMAKE_PREFIX_PATH ${_contrib_lua_path} ${_contrib_lua_libpath})
		set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} PARENT_SCOPE)
	endif()
endif()

# Building the updater when using non-default paths (especially absolute paths)
# is not recommended as it most likely won't work
set(RTTR_BUILD_UPDATER_DEF ON)
foreach(curDirVar BINDIR DATADIR LIBDIR DOCDIR DRIVERDIR)
    if(IS_ABSOLUTE ${RTTR_${curDirVar}})
        set(RTTR_BUILD_UPDATER_DEF OFF)
    endif()
endforeach()
option(RTTR_BUILD_UPDATER "Build auto-updater. Not advised when changing the default paths as the updater is configured specifically for the official builds." ${RTTR_BUILD_UPDATER_DEF})

add_subdirectory(glad)
add_library(kaguya INTERFACE)
target_include_directories(kaguya SYSTEM INTERFACE kaguya/include)
add_subdirectory(libendian)
set(RTTR_BUILD_LOBBY_CLIENT ON)
add_subdirectory(liblobby)

option(RTTR_USE_SYSTEM_SAMPLERATE OFF "Use system installed libsamplerate. Fails if not found!")
if(RTTR_USE_SYSTEM_SAMPLERATE)
	find_package(Samplerate 0.1.9 REQUIRED)
	unset(samplerate_LIB_DEPENDS CACHE)
	add_library(samplerate INTERFACE)
	target_link_libraries(samplerate INTERFACE Samplerate::Samplerate)
else()
	set(LIBSAMPLERATE_TESTS OFF CACHE INTERNAL "")
	set(LIBSAMPLERATE_INSTALL OFF CACHE INTERNAL "")
	add_subdirectory(libsamplerate)
endif()

add_subdirectory(libsiedler2)
add_subdirectory(libutil)
add_subdirectory(mygettext)
add_subdirectory(s25edit)
if(RTTR_BUILD_UPDATER)
    add_subdirectory(s25update)
endif()
add_library(turtle INTERFACE)
target_include_directories(turtle SYSTEM INTERFACE turtle/include)
target_compile_definitions(turtle INTERFACE MOCK_NO_AUTO_PTR)
