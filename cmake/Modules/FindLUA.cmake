# CUSTOM(!!!) module for finding lua.
# Sets LUA_LIBRARY AND LUA_INCLUDE_DIR
# For MSVC it also sets LUA_DLL
# Requires LUA_VERSION to be set to MAJORMINOR (e.g. 52)

if(WIN32)
	if("${PLATFORM_ARCH}" STREQUAL "i386")
		SET(LUA_DIR "${CMAKE_SOURCE_DIR}/contrib/lua/win32")
	ELSE()
		SET(LUA_DIR "${CMAKE_SOURCE_DIR}/contrib/lua/win64")
	endif()
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
	if("${PLATFORM_ARCH}" STREQUAL "i386")
		SET(LUA_DIR "${CMAKE_SOURCE_DIR}/contrib/lua/lin32")
	ELSE()
		SET(LUA_DIR "${CMAKE_SOURCE_DIR}/contrib/lua/lin64")
	endif()
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
	SET(LUA_DIR "${CMAKE_SOURCE_DIR}/contrib/lua/mac")
endif()
set(LUA_DIR ${LUA_DIR} CACHE PATH "Path to default lua directory")
mark_as_advanced(LUA_DIR)

if(NOT LUA_LIBRARY OR NOT LUA_INCLUDE_DIR)
    # Defaults
    IF(NOT LUA_INCLUDE_DIR)
        set(LUA_INCLUDE_DIR "${LUA_DIR}/include")
    endif()
    IF(NOT DEFINED LUA_LIBRARY)
        if(MSVC)
            FIND_LIBRARY(LUA_LIBRARY NAMES "lua${LUA_VERSION}")
        ELSE ()
            SET(LUA_LIBRARY "${LUA_DIR}/liblua${LUA_VERSION}.a")
        endif()
    endif()

    if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "FreeBSD")
        SET(LUA_LIBRARY "/usr/local/lib/liblua${LUA_VERSION}.a")
        set(LUA_INCLUDE_DIR "/usr/local/include/lua${LUA_VERSION}")
    endif()

    if(NOT EXISTS "${LUA_LIBRARY}")
        MESSAGE(STATUS "Checking LUA library for default fallbacks")
        string(REGEX REPLACE "(.)(.)" "\\1.\\2" LUA_DOT_VERSION "${LUA_VERSION}")

        SET(LUA_LIBRARY "/usr/lib/liblua${LUA_DOT_VERSION}.a")
        set(LUA_INCLUDE_DIR "/usr/include/lua${LUA_DOT_VERSION}")

        if(NOT EXISTS "${LUA_LIBRARY}")
            SET(LUA_LIBRARY "/usr/lib/liblua.so")
            set(LUA_INCLUDE_DIR "/usr/include/")
        endif()

        if(NOT EXISTS "${LUA_LIBRARY}")
            SET(LUA_LIBRARY "/usr/local/lib/liblua-${LUA_DOT_VERSION}.so")
            set(LUA_INCLUDE_DIR "/usr/local/include/")
        endif()
    endif()

    set(LUA_INCLUDE_DIR ${LUA_INCLUDE_DIR} CACHE PATH "Path to lua includes")
    set(LUA_LIBRARY ${LUA_LIBRARY} CACHE PATH "Path to lua library")
endif()

if(MSVC)
    SET(LUA_DLL "${LUA_DIR}/lua${LUA_VERSION}.dll" CACHE PATH "Path to lua dynamic library")
    mark_as_advanced(LUA_DLL)
    if(NOT EXISTS "${LUA_DLL}")
        MESSAGE(FATAL_ERROR "Lua dynamic library not found at ${LUA_DLL}")
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Lua DEFAULT_MSG LUA_INCLUDE_DIR LUA_LIBRARY)

MARK_AS_ADVANCED(
  LUA_INCLUDE_DIR
  LUA_LIBRARY
)
