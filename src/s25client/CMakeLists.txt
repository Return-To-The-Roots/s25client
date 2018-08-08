# Add Icon to Application
if(WIN32)
    if(CMAKE_HOST_WIN32)
        SET(s25client_RC "${CMAKE_SOURCE_DIR}/win32/s25client.rc")
    else()
        ADD_DEFINITIONS("-DAFX_TARG_DEU")

        ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/s25client.res.o
                           COMMAND ${CMAKE_RC_COMPILER} -I${CMAKE_SOURCE_DIR}/win32/ -i${CMAKE_SOURCE_DIR}/win32/s25client.rc -o ${CMAKE_CURRENT_BINARY_DIR}/s25client.res.o)

        SET(s25client_RC ${CMAKE_CURRENT_BINARY_DIR}/s25client.res.o)
    endif()
ELSE()
	SET(s25client_RC "")
ENDIF()

ADD_EXECUTABLE(s25client s25client.cpp ${s25client_RC})
TARGET_LINK_LIBRARIES(s25client s25Main)

if(APPLE)
	TARGET_LINK_LIBRARIES(s25client ${SDL_LIBRARY} SDLmain)
elseif(WIN32)
	TARGET_LINK_LIBRARIES(s25client ole32 ws2_32 shlwapi imagehlp)
    if(MSVC)
        set_target_properties(s25client PROPERTIES LINK_FLAGS /MANIFEST:NO)
    endif()
    if(CMAKE_COMPILER_IS_GNUCXX)
        set_target_properties(s25client PROPERTIES LINK_FLAGS -Wl,--stack,8388608)
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	TARGET_LINK_LIBRARIES(s25client pthread)
elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
	TARGET_LINK_LIBRARIES(s25client execinfo)
ENDif()

if(MSVC)
	if(CMAKE_VERSION VERSION_LESS 3.6)
		CMAKE_POLICY(SET CMP0026 OLD) # Required for use of LOCATION_*
		INCLUDE(CreateLaunchers)
		CREATE_DEFAULT_TARGET_LAUNCHER(s25client)
	else()
		set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT s25client)
	endif()
ENDIF()

if(WIN32)
    include(GatherDll)
    gather_dll_copy(s25client)
endif()

INSTALL(TARGETS s25client RUNTIME DESTINATION ${RTTR_BINDIR})
