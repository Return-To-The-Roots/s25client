################################################################################
### $Id: CMakeMacroSetCCache.cmake 7115 2011-04-02 14:16:37Z FloSoft $
################################################################################

MACRO(SET_CCACHE var file)
	IF(EXISTS /usr/lib/ccache_lipo/${file})
		SET(${var} /usr/lib/ccache_lipo/${file})
	ELSE(EXISTS /usr/lib/ccache_lipo/${file})
		IF(EXISTS /usr/lib/ccache-lipo/${file})
			SET(${var} /usr/lib/ccache-lipo/${file})
		ELSE(EXISTS /usr/lib/ccache-lipo/${file})
			IF(EXISTS /usr/lib/ccache/${file})
				SET(${var} /usr/lib/ccache/${file})
			ELSE(EXISTS /usr/lib/ccache/${file})
				IF(EXISTS /opt/local/libexec/ccache/${file})
					SET(${var} /opt/local/libexec/ccache/${file})
				ELSE(EXISTS /opt/local/libexec/ccache/${file})
					SET(${var} ${file})
				ENDIF(EXISTS /opt/local/libexec/ccache/${file})
			ENDIF(EXISTS /usr/lib/ccache/${file})
		ENDIF(EXISTS /usr/lib/ccache-lipo/${file})
	ENDIF(EXISTS /usr/lib/ccache_lipo/${file})
	
	MESSAGE(STATUS "Set ${var} to ${${var}}")
ENDMACRO(SET_CCACHE)

