SET(PATHS "")

FOREACH(I ${SPLIT})
        IF(IS_DIRECTORY "${I}" )
                IF("${PATHS}" STREQUAL "")
                        SET(PATHS "${I}")
               	ELSE("${PATHS}" STREQUAL "")
                	SET(PATHS "${PATHS} ${I}")
        	ENDIF("${PATHS}" STREQUAL "")
	ENDIF(IS_DIRECTORY "${I}")
ENDFOREACH(I)

