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

# If not set we set hints for the LUA library. Those can be overwritten by
# setting CMAKE_PREFIX_PATH properly
# Note: We only set those for the libraries we use for official builds. Normally
# those should be on the build server but we make them available here for convenience.

if(NOT ENV{LUA_DIR})
	if(NOT EXISTS "${CMAKE_SOURCE_DIR}/contrib/lua/include/lua.h")
		message(WARNING "Could not find lua.h in contrib")
	endif()
	if(MSVC)
		# Library is in contrib archive which is in prefix path.
		# We just need to the includes to be found
		list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/contrib/lua")
	elseif(WIN32 OR CYGWIN)
		list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/contrib/lua")
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			set(ENV{LUA_DIR} "${CMAKE_SOURCE_DIR}/contrib/lua/win64")
		elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
			set(ENV{LUA_DIR} "${CMAKE_SOURCE_DIR}/contrib/lua/win32")
		endif()
	elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
		if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "i.86")
			list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/contrib/lua")
			set(ENV{LUA_DIR} "${CMAKE_SOURCE_DIR}/contrib/lua/lin32")
		elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "amd64|AMD64|x86_64")
			list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/contrib/lua")
			set(ENV{LUA_DIR} "${CMAKE_SOURCE_DIR}/contrib/lua/lin64")
		endif()
	elseif(APPLE)
		list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/contrib/lua")
		set(ENV{LUA_DIR} "${CMAKE_SOURCE_DIR}/contrib/lua/mac")
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
target_include_directories(gamedata SYSTEM PUBLIC ${RTTR_CONTRIB_DIR}/kaguya/include ${LUA_INCLUDE_DIR} PRIVATE ${UTF8_INCLUDE_DIR})
TARGET_LINK_LIBRARIES(gamedata PUBLIC
	s25util
	mygettext
	s25Common
	rttrConfig
	${LUA_LIBRARIES}
)

