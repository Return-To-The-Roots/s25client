ADD_DEFINITIONS(-DNOMINMAX)
SET(Boost_USE_STATIC_LIBS TRUE)

IF(NOT MSVC)
    include(CheckCXXSourceCompiles)
	# set compiler flags
    set(ADDITIONAL_FLAGS -ffast-math -mmmx -msse -fomit-frame-pointer -ggdb)
    # Bug in MinGW with mfpmath=sse
    set(CMAKE_REQUIRED_FLAGS "-msse -mfpmath=sse")
    check_cxx_source_compiles("
        #include <cmath>
        int main() {}" FPMATH_SUPPORTED)
    set(CMAKE_REQUIRED_FLAGS )
    if(FPMATH_SUPPORTED)
        list(APPEND ADDITIONAL_FLAGS -mfpmath=sse)
    endif()
	FORCE_ADD_FLAGS(CMAKE_C_FLAGS ${ADDITIONAL_FLAGS})
	FORCE_ADD_FLAGS(CMAKE_CXX_FLAGS ${ADDITIONAL_FLAGS})

	ADD_DEFINITIONS(-D__USE_W32_SOCKETS)
    # If using MinGW under windows we detect this and add it to the CMAKE_PREFIX_PATH
    if(${CMAKE_CXX_COMPILER} MATCHES "MinGW/bin/")
        get_filename_component(MINGW_BIN_PATH ${CMAKE_CXX_COMPILER} DIRECTORY)
        get_filename_component(MINGW_PATH ${MINGW_BIN_PATH} DIRECTORY)
        # Note: Do not add the main MinGW path (e.g. C:\MinGW) as adding C:\MinGW\include to the system include dirs causes GCC failures
        list(APPEND CMAKE_PREFIX_PATH ${MINGW_PATH}/mingw32)
    endif()
ELSE()
	# disable MSVC "use secure function"
	ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS /wd"4250")
	# 'class1' : inherits 'class2::member' via dominance (virtual inheritance related)
	add_compile_options(/wd"4250")
	# conditional expr is constant
	add_compile_options(/wd"4127")
	# unreferenced formal parameter
	add_compile_options(/wd"4100")
	# assignment operator could not be created
	add_compile_options(/wd"4512")
	add_compile_options(/w34062) # Enum not handled in switch
	# disable MSVC posix functions
	ADD_DEFINITIONS(-D_CRT_NONSTDC_NO_DEPRECATE -D_WINSOCK_DEPRECATED_NO_WARNINGS)
     # systemintern functions for faster code; Optimize whole program
	add_flags(CMAKE_CXX_FLAGS_RELEASE /Oi /GL)
	add_flags(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL /Oi /GL)
	# Strip unused symbols and us COMDAT folding
	add_flags(CMAKE_EXE_LINKER_FLAGS_RELEASE /OPT:REF /OPT:ICF)
	add_flags(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL /OPT:REF /OPT:ICF)
	add_flags(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO /OPT:REF /OPT:ICF) 
	# Add optimized debugging features
	IF (MSVC_VERSION GREATER 1800) #VS13
		add_compile_options(/d2Zi+)
	ELSEIF (NOT(MSVC_VERSION LESS 1800)) # VS12
		add_compile_options(/Zo)
	ENDIF()
ENDIF()
