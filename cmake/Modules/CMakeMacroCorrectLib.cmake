#################################################################################
### $Id: CMakeMacroCorrectLib.cmake 6807 2010-10-18 14:12:04Z FloSoft $
#################################################################################

INCLUDE(CMakeMacroAddFlags)

MACRO(CORRECT_LIB library name)
	IF ( CROSSCOMPILE AND "${COMPILEFOR}" STREQUAL "apple")
		WHILE ( IS_DIRECTORY ${${library}})
		
			SET(TMP ${${library}})
			STRING(REPLACE ";" " " TMP "${TMP}")
			SEPARATE_ARGUMENTS(TMP)

			SET(paths "")
			SET(new_library-NOTFOUND "not found")
			FOREACH(path ${TMP})
				IF(IS_DIRECTORY "${path}" )
					IF(EXISTS ${path}/${name})
						SET(new_library ${path}/${name})
						SET(new_library-NOTFOUND)
					ENDIF(EXISTS ${path}/${name})
				ENDIF(IS_DIRECTORY "${path}")
			ENDFOREACH(path ${TMP})
			
			IF(NOT new_library-NOTFOUND)
				MESSAGE(STATUS "Found ${name}: ${new_library}")
				SET(${library} ${new_library})
			ENDIF(NOT new_library-NOTFOUND)
		ENDWHILE ( IS_DIRECTORY ${${library}} )
	ELSE ( CROSSCOMPILE AND "${COMPILEFOR}" STREQUAL "apple")
				MESSAGE(STATUS "Found ${name}: ${${library}}")
	ENDIF ( CROSSCOMPILE AND "${COMPILEFOR}" STREQUAL "apple")
ENDMACRO(CORRECT_LIB)
