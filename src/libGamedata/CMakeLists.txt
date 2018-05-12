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

if(NOT ENV{LUA_DIR})
	if(MSVC)
		# Library is in contrib archive which is in prefix path.
		# We just need to set LUA_DIR so the includes can be found
		set(_lua_dir "${CMAKE_SOURCE_DIR}/contrib/lua")
	elseif(WIN32 OR CYGWIN)
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			set(_lua_dir "${CMAKE_SOURCE_DIR}/contrib/lua/win64")
		elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
			set(_lua_dir "${CMAKE_SOURCE_DIR}/contrib/lua/win32")
		endif()
	elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
		if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "i.86")
			set(_lua_dir "${CMAKE_SOURCE_DIR}/contrib/lua/lin32")
		elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "amd64|AMD64|x86_64")
			set(_lua_dir "${CMAKE_SOURCE_DIR}/contrib/lua/lin64")
		endif()
	elseif(APPLE)
		set(_lua_dir "${CMAKE_SOURCE_DIR}/contrib/lua/mac")
	endif()
	
	# FindLua searches for hints in here
	SET(ENV{LUA_DIR} ${_lua_dir})
	unset(_lua_dir)
endif()

if(CMAKE_VERSION VERSION_LESS 3.2)
	find_package(Lua 5.2.1 EXACT REQUIRED)
else()
	find_package(Lua 5.2 EXACT REQUIRED)
endif()

include(GatherDll)
gather_dll(LUA)

include_directories(${UTF8_INCLUDE_DIR})
ADD_LIBRARY(gamedata STATIC ${SOURCES_OTHER} ${SOURCES_SUBDIRS})
target_include_directories(gamedata PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(gamedata SYSTEM PUBLIC ${LUA_INCLUDE_DIR} ${RTTR_CONTRIB_DIR}/kaguya/include)
TARGET_LINK_LIBRARIES(gamedata
	s25util
	mygettext
	s25Common
	rttrConfig
	lua::lua
)

