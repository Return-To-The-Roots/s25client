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

SET(LUA_VERSION "52")

FIND_PACKAGE(LUA REQUIRED)

include(GatherDll)
gather_dll(LUA)

include_directories(${UTF8_INCLUDE_DIR})
ADD_LIBRARY(gamedata STATIC ${SOURCES_OTHER} ${SOURCES_SUBDIRS})
target_include_directories(gamedata PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(gamedata SYSTEM PUBLIC ${LUA_INCLUDE_DIR} ${RTTR_CONTRIB_DIR}/kaguya/include)
TARGET_LINK_LIBRARIES(gamedata
	PUBLIC s25util
	PUBLIC mygettext
	PUBLIC s25Common
	PUBLIC rttrConfig
	PUBLIC ${LUA_LIBRARY}
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	TARGET_LINK_LIBRARIES(gamedata PUBLIC dl)
ENDif()