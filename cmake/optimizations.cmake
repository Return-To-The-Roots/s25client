include(CheckAndAddFlag)

if(NOT MSVC)
    if(NOT RTTR_OPTIMZATION_VECTOR_EXT_DEFAULT)
        set(RTTR_OPTIMZATION_VECTOR_EXT_DEFAULT SSE)
    endif()
    set(RTTR_OPTIMZATION_VECTOR_EXT ${RTTR_OPTIMZATION_VECTOR_EXT_DEFAULT} CACHE STRING "Vector extension to use")
    set(RTTR_OPTIMZATION_TUNE "${RTTR_OPTIMZATION_TUNE_DEFAULT}" CACHE STRING "Target architecture to tune for (e.g. core2, prescott)")
    # Set allowed options
    set_property(CACHE RTTR_OPTIMZATION_VECTOR_EXT PROPERTY STRINGS MMX SSE SSE2)
    
    CheckAndAddFlags(-O2 -ffast-math -fomit-frame-pointer -mtune=generic)

    if(${RTTR_OPTIMZATION_VECTOR_EXT} MATCHES SSE|MMX)
        CheckAndAddFlag(-mmmx)
    endif()
    if(${RTTR_OPTIMZATION_VECTOR_EXT} MATCHES SSE)
        CheckAndAddFlags(-msse -mfpmath=sse)
    endif()
    if(${RTTR_OPTIMZATION_VECTOR_EXT} MATCHES SSE2)
        CheckAndAddFlags(-msse2)
    endif()
    
    if(RTTR_OPTIMZATION_TUNE)
        # Adding multiple mtune flags just uses the last one, so adding mtune=generic above is ok
        CheckAndAddFlags(-mtune=${RTTR_OPTIMZATION_TUNE})
    endif()
    
    if(APPLE)
        # ppc only?
        IF(CMAKE_OSX_ARCHITECTURES MATCHES "^ppc[^;]*$")
            CheckAndAddFlags(-faltivec -maltivec) # Clang and GCC version
        ENDIF()
    endif()
endif()
