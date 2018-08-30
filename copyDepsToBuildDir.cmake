# Needs to be configured with @ONLY and called with -DCMAKE_BUILD_TYPE=...

set(CMAKE_HOST_UNIX "@CMAKE_HOST_UNIX@")
set(CMAKE_SOURCE_DIR "@CMAKE_SOURCE_DIR@")
SET(RTTR_DATADIR "@RTTR_DATADIR@")
SET(RTTR_GAMEDIR "@RTTR_GAMEDIR@")
set(RTTR_OUTPUT_DIR "@RTTR_OUTPUT_DIR@")
set(RTTR_OUTPUT_DIR_RELEASE "@RTTR_OUTPUT_DIR_RELEASE@")
set(RTTR_OUTPUT_DIR_DEBUG "@RTTR_OUTPUT_DIR_DEBUG@")
set(RTTR_OUTPUT_DIR_RELWITHDEBINFO "@RTTR_OUTPUT_DIR_RELWITHDEBINFO@")
set(RTTR_OUTPUT_DIR_MINSIZEREL "@RTTR_OUTPUT_DIR_MINSIZEREL@")

################################################################################
# Create symlinks for easier debugging
################################################################################

function(symlinkFolder dst symLinkPath)
	IF(NOT EXISTS "${symLinkPath}")
		IF(EXISTS "${dst}")
			unset(symlink_failed)
			message(STATUS "Creating symlink: '${symLinkPath}' -> '${dst}'")
			execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink "${dst}" "${symLinkPath}" RESULT_VARIABLE symlink_failed)
			if(symlink_failed)
				message(FATAL_ERROR "Creating symlink failed")
			endif()
		ELSE()
			message(WARNING "Directory '${dst}' missing!")
		ENDIF()
	ENDIF()
endfunction()

if(NOT CMAKE_BUILD_TYPE)
    set(CUR_OUTPUT_DIR ${RTTR_OUTPUT_DIR})
else()
    string(TOUPPER ${CMAKE_BUILD_TYPE} UPPER_BUILD_TYPE)
    set(CUR_OUTPUT_DIR ${RTTR_OUTPUT_DIR_${UPPER_BUILD_TYPE}})
endif()

IF(CMAKE_HOST_UNIX)
    message(STATUS "Creating symlinks in ${CUR_OUTPUT_DIR} to ease debugging.")
	file(MAKE_DIRECTORY "${CUR_OUTPUT_DIR}/${RTTR_DATADIR}")
	symlinkFolder("${CMAKE_SOURCE_DIR}/RTTR" "${CUR_OUTPUT_DIR}/${RTTR_DATADIR}/RTTR")
	file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/S2")
	symlinkFolder("${CMAKE_SOURCE_DIR}/S2/DATA" "${CUR_OUTPUT_DIR}/${RTTR_GAMEDIR}/DATA")
	symlinkFolder("${CMAKE_SOURCE_DIR}/S2/GFX" "${CUR_OUTPUT_DIR}/${RTTR_GAMEDIR}/GFX")
ELSE()
	message(STATUS "Host system is not Unix. Will use copies instead of symlinks to init ${CUR_OUTPUT_DIR}.")
    file(COPY "${CMAKE_SOURCE_DIR}/RTTR" DESTINATION "${CUR_OUTPUT_DIR}"
		 PATTERN "*.po" EXCLUDE
		 PATTERN "*.pot" EXCLUDE
		 PATTERN ".*" EXCLUDE
	)
    set(S2_GAME_PATHS ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/S2)
    if(NOT EXISTS ${CUR_OUTPUT_DIR}/DATA)
        find_path(S2_DATA_DIR DATA/IO.LST PATHS ${S2_GAME_PATHS})
        if(S2_DATA_DIR)
            file(COPY ${S2_DATA_DIR}/DATA DESTINATION ${CUR_OUTPUT_DIR})
        endif()
    endif()
    if(NOT EXISTS ${CUR_OUTPUT_DIR}/GFX)
        find_path(S2_GFX_DIR GFX/PALETTE/PAL5.BBM PATHS ${S2_GAME_PATHS})
        if(S2_GFX_DIR)
            file(COPY ${S2_GFX_DIR}/GFX DESTINATION ${CUR_OUTPUT_DIR})
        endif()
    endif()
ENDIF()