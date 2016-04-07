FORCE_ADD_FLAGS(CMAKE_C_FLAGS -DNOMINMAX)
FORCE_ADD_FLAGS(CMAKE_CXX_FLAGS -DNOMINMAX)
SET(Boost_USE_STATIC_LIBS TRUE)

IF(NOT MSVC)
	# set compiler flags
	FORCE_ADD_FLAGS(CMAKE_C_FLAGS -ffast-math -mmmx -msse -mfpmath=sse -fomit-frame-pointer -ggdb)
	FORCE_ADD_FLAGS(CMAKE_CXX_FLAGS -ffast-math -mmmx -msse -mfpmath=sse -fomit-frame-pointer -ggdb)

	# bugfix for cygwin
	#ADD_DEFINITIONS(-D_WIN32 -D__USE_W32_SOCKETS)

	FORCE_ADD_FLAGS(CMAKE_C_FLAGS -D_WIN32 -D__USE_W32_SOCKETS)
	FORCE_ADD_FLAGS(CMAKE_CXX_FLAGS -D_WIN32 -D__USE_W32_SOCKETS)
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
