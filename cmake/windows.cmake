ADD_DEFINITIONS(-DNOMINMAX)
# Enforce unicode
ADD_DEFINITIONS(-DUNICODE -D_UNICODE)
SET(Boost_USE_STATIC_LIBS ON CACHE BOOL "Use static boost libs")

IF(NOT MSVC)
	ADD_DEFINITIONS(-D__USE_W32_SOCKETS)
    # If using MinGW under windows we detect this and add it to the CMAKE_PREFIX_PATH
    if(${CMAKE_CXX_COMPILER} MATCHES "MinGW/bin/")
        get_filename_component(MINGW_BIN_PATH ${CMAKE_CXX_COMPILER} DIRECTORY)
        get_filename_component(MINGW_PATH ${MINGW_BIN_PATH} DIRECTORY)
        # Note: Do not add the main MinGW path (e.g. C:\MinGW) as adding C:\MinGW\include to the system include dirs causes GCC failures
        list(APPEND CMAKE_PREFIX_PATH ${MINGW_PATH}/mingw32)
    endif()
ELSE()
	# Add optimized debugging features
	add_compile_options(/d2Zi+) # added in MSVC 2015
ENDIF()
