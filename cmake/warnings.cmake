option(RTTR_ENABLE_WERROR "Build with warnings turned into errors" ON)
if(MSVC)
	add_compile_options(/W3)
	add_compile_options(/MP) # parallel compilation
	# Signed/Unsigned operations
	add_compile_options(/w34389)
	# 'class1' : inherits 'class2::member' via dominance (virtual inheritance related)
	add_compile_options(/wd"4250")
	# conditional expr is constant
	add_compile_options(/wd"4127")
	# unreferenced formal parameter
	add_compile_options(/wd"4100")
	# assignment operator could not be created
	add_compile_options(/wd"4512")
	add_compile_options(/w34062) # Enum not handled in switch
	# disable MSVC "use secure function"
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS /wd"4250")
	# disable MSVC posix functions
	add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE -D_WINSOCK_DEPRECATED_NO_WARNINGS)
	if(RTTR_ENABLE_WERROR)
		add_compile_options(/WX) # warning = error
	endif()
else()
  include(CheckAndAddFlag)
	CheckAndAddFlag("-Wall")
	if(RTTR_ENABLE_WERROR)
		CheckAndAddFlag("-Werror")
	endif()
	CheckAndAddFlags("-pedantic" "-Wpedantic")
	CheckAndAddFlag("-Wparentheses")
	CheckAndAddFlag("-Wno-error=type-limits")
	CheckAndAddFlag("-Wfloat-conversion")
	CheckAndAddFlag("-Wno-long-long")
	CheckAndAddFlag("-Wno-deprecated-register")
	CheckAndAddFlag("-Wno-unknown-pragmas")
	CheckAndAddFlag("-fno-strict-aliasing")
	CheckAndAddFlag("-Qunused-arguments")

	CHECK_CXX_SOURCE_COMPILES("
		#if __cplusplus >= 201103L
		int main() {}
		#endif" COMPILER_IN_CXX11_MODE)
	if(COMPILER_IN_CXX11_MODE)
		CheckAndAddFlags("-Wsuggest-override" "-Wno-error=suggest-override")
	else()
		add_definitions(-Doverride=)
	    CheckAndAddFlag("-Wno-c++11-extensions")
    	# Variadic macros are part of C99 but supported by all big compilers in C++03
	    CheckAndAddFlag("-Wno-variadic-macros")
	    CheckAndAddFlag("-Wno-c99-extensions")
	    # For Boost < 1.59 (static-assert emulation)
	    CheckAndAddFlag("-Wno-unused-local-typedef")
	endif()
endif()
