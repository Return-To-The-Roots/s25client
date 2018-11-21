SET(SOURCES_SUBDIRS )
MACRO(AddDirectory dir)
	FILE(GLOB SUB_FILES ${dir}/*.cpp ${dir}/*.h ${dir}/*.hpp ${dir}/*.tpp)
	SET(SOURCES_SUBDIRS ${SOURCES_SUBDIRS} ${SUB_FILES})
	SOURCE_GROUP(${dir} FILES ${SUB_FILES})
ENDMACRO()

AddDirectory(gameData)
AddDirectory(lua)

FILE(GLOB SOURCES_OTHER *.cpp *.h)
SOURCE_GROUP(other FILES ${SOURCES_OTHER})

################################################################################
# LUA
################################################################################

# Here we set hints to lua libraries used for official builds Normally those should be
# on the build server but we make them available here for convenience.
# As this will be appended to CMAKE_PREFIX_PATH one can use other version by setting
# that appropriately

set(_contrib_lua_path ${CMAKE_SOURCE_DIR}/external/lua)

if(MSVC)
	# Library is in contrib archive which is in prefix path.
	# We just need the includes (which we did not duplicate in the msvc-contrib
	list(APPEND CMAKE_PREFIX_PATH ${_contrib_lua_path})
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
	else()
		list(APPEND CMAKE_PREFIX_PATH ${_contrib_lua_path} ${_contrib_lua_libpath})
	endif()
endif()

# CMake < 3.2 treats version 5.2.x as not equal to 5.2 but we want to allow all 5.2 versions
if(CMAKE_VERSION VERSION_LESS 3.2)
	find_package(Lua 5.2 REQUIRED)
	if(LUA_FOUND AND NOT LUA_VERSION_STRING VERSION_LESS 5.3)
		message(FATAL_ERROR "Lua version ${LUA_VERSION_STRING} found but required is 5.2")
	endif()
else()
	find_package(Lua 5.2 EXACT REQUIRED)
endif()

include(GatherDll)
gather_dll(LUA)

ADD_LIBRARY(gamedata STATIC ${SOURCES_OTHER} ${SOURCES_SUBDIRS})
target_include_directories(gamedata PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(gamedata SYSTEM PUBLIC ${LUA_INCLUDE_DIR} PRIVATE ${UTF8_INCLUDE_DIR})
TARGET_LINK_LIBRARIES(gamedata PUBLIC
	s25util
	mygettext
	s25Common
	rttrConfig
	kaguya
	${LUA_LIBRARIES}
)
