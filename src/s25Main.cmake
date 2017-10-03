FILE(GLOB SOURCES_OTHER *.cpp *.h)
LIST(APPEND SOURCES_OTHER ${COMMON_SRC})
SOURCE_GROUP(other FILES ${SOURCES_OTHER})

SET(SOURCES_SUBDIRS )
MACRO(AddDirectory dir)
	FILE(GLOB SUB_FILES ${dir}/*.cpp ${dir}/*.h ${dir}/*.hpp ${dir}/*.tpp)
	SET(SOURCES_SUBDIRS ${SOURCES_SUBDIRS} ${SUB_FILES})
	SOURCE_GROUP(${dir} FILES ${SUB_FILES})
ENDMACRO()

AddDirectory(addons)
AddDirectory(ai)
AddDirectory(animation)
AddDirectory(buildings)
AddDirectory(controls)
AddDirectory(desktops)
AddDirectory(drivers)
AddDirectory(factories)
AddDirectory(figures)
AddDirectory(gameData)
AddDirectory(gameTypes)
AddDirectory(helpers)
AddDirectory(ingameWindows)
AddDirectory(lua)
AddDirectory(mapGenerator)
AddDirectory(nodeObjs)
AddDirectory(notifications)
AddDirectory(ogl)
AddDirectory(pathfinding)
AddDirectory(postSystem)
AddDirectory(random)
AddDirectory(world)

INCLUDE(AddFileDependencies)
ADD_FILE_DEPENDENCIES(${CMAKE_CURRENT_SOURCE_DIR}/RTTR_Version.cpp ${CMAKE_BINARY_DIR}/build_version_defines.h)

SET(s25Main_SRCS
	${SOURCES_OTHER}
	${SOURCES_SUBDIRS}
)

include_directories(${UTF8_INCLUDE_DIR})
ADD_LIBRARY(s25Main STATIC ${s25Main_SRCS})
ADD_DEPENDENCIES(s25Main updateversion)
target_include_directories(s25Main PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
TARGET_LINK_LIBRARIES(s25Main
	siedler2
	lobby_c
	s25util
	mygettext
	${BZIP2_LIBRARIES}
	${OPENGL_gl_LIBRARY}
	${LUA_LIBRARY}
	${Boost_LIBRARIES}
)

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
	TARGET_LINK_LIBRARIES(s25Main dl)
ENDif()

if(MSVC)
	ADD_CUSTOM_COMMAND(TARGET s25Main
						PRE_BUILD
						COMMAND "$<TARGET_FILE:version>" "${CMAKE_SOURCE_DIR}"
						DEPENDS version
						WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
	)
    if(RTTR_BINARIES_TO_COPY)
        foreach(file_i ${RTTR_BINARIES_TO_COPY})
            add_custom_command(TARGET s25Main POST_BUILD COMMAND ${CMAKE_COMMAND} ARGS -E copy ${file_i} ${CMAKE_BINARY_DIR})
        endforeach()
    endif()

	ADD_CUSTOM_COMMAND(TARGET s25Main POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/RTTR" "${CMAKE_BINARY_DIR}/RTTR")
ENDIF()