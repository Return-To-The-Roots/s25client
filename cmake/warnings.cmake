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
	# disable warning 4267: 'var' : conversion from 'size_t' to 'type', possible loss of data
	add_compile_options(/wd4267)
	# disable MSVC "use secure function"
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS /wd"4250")
	# disable MSVC posix functions
	add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE -D_WINSOCK_DEPRECATED_NO_WARNINGS)
	if(RTTR_ENABLE_WERROR)
		add_compile_options(/WX) # warning = error
	endif()
else()
  include(CheckAndAddFlag)
	add_compile_options(-Wall)
	if(RTTR_ENABLE_WERROR)
		add_compile_options(-Werror)
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

	if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
		add_compile_options(
			"$<$<COMPILE_FEATURES:cxx_override>:-Wsuggest-override -Wno-error=suggest-override>"
    	# Variadic macros are part of C99 but supported by all big compilers in C++03
			"$<$<NOT:$<COMPILE_FEATURES:cxx_std_11>>:-Wno-c++11-extensions -Wno-variadic-macros-Wno-c99-extensions>"
	    # For Boost < 1.59 (static-assert emulation)
			"$<$<NOT:$<COMPILE_FEATURES:cxx_static_assert>>:-Wno-unused-local-typedef>"
		)
	endif()
endif()
