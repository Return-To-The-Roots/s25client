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
        list(APPEND CMAKE_PREFIX_PATH ${MINGW_PATH})
    endif()
ELSE(NOT MSVC)
	# disable MSVC "use secure function"
	ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS /wd"4250")
	# 'class1' : inherits 'class2::member' via dominance (virtual inheritance related)
	ADD_DEFINITIONS(/wd"4250")
	# conditional expr is constant
	ADD_DEFINITIONS(/wd"4127")
	# unreferenced formal parameter
	ADD_DEFINITIONS(/wd"4100")
	# assignment operator could not be created
	ADD_DEFINITIONS(/wd"4512")
	# disable MSVC posix functions
	ADD_DEFINITIONS(-D_CRT_NONSTDC_NO_DEPRECATE)
	ADD_DEFINITIONS(-D_WINSOCK_DEPRECATED_NO_WARNINGS)
	ADD_DEFINITIONS(/w34062) # Enum not handled in switch
	SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Oi /GL") # systemintern functions for faster code; Optimize whole program
	# Strip unused symbols and us COMDAT folding
	SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /OPT:REF /OPT:ICF")
	SET(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} /OPT:REF /OPT:ICF")
	SET(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /OPT:REF /OPT:ICF") 
	# Add optimized debugging features
	IF (MSVC_VERSION GREATER 1800) #VS13
		ADD_DEFINITIONS(/d2Zi+)
	ELSEIF (NOT(MSVC_VERSION LESS 1800)) # VS12
		ADD_DEFINITIONS(/Zo)
	ENDIF()
ENDIF(NOT MSVC)
