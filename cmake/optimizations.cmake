# Copyright (C) 2018 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

if(MSVC)
    include(AppendToStringUnique)
    # systemintern functions for faster code; Optimize whole program
    append_to_string_unique(CMAKE_CXX_FLAGS_RELEASE /Oi /GL)
    # Strip unused symbols and us COMDAT folding
    append_to_string_unique(CMAKE_EXE_LINKER_FLAGS_RELEASE /OPT:REF /OPT:ICF)
    append_to_string_unique(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL /OPT:REF /OPT:ICF)
    append_to_string_unique(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO /OPT:REF /OPT:ICF)
else()
    include(CheckAndAddFlag)
    include(SetIfUnset)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
        include(ArmConfig)
        set(RTTR_TARGET_BOARD "OFF" CACHE STRING "Set to your target device to enable extra optimization flags. Available: ${RTTR_AVAILABLE_ARM_CFGS}")
        set_property(CACHE RTTR_TARGET_BOARD PROPERTY STRINGS ${RTTR_AVAILABLE_ARM_CFGS})
        if(RTTR_TARGET_BOARD)
            get_arm_tune(arm_tune_default "${RTTR_TARGET_BOARD}")
            set(RTTR_OPTIMIZATION_TUNE ${arm_tune_default} CACHE STRING "" FORCE)
            if(NOT RTTR_TARGET_BOARD STREQUAL "Generic")
                get_arm_flags(arm_flags "${RTTR_TARGET_BOARD}")
                CheckAndAddFlags(${arm_flags})
            endif()
        endif()
        # Exception support is required. See #855
        CheckAndAddFlags(--exceptions)
    else()
        set_if_unset(RTTR_OPTIMIZATION_VECTOR_EXT_DEFAULT SSE2)
        set(RTTR_OPTIMIZATION_VECTOR_EXT ${RTTR_OPTIMIZATION_VECTOR_EXT_DEFAULT} CACHE STRING "Vector extension to use")
        # Set allowed options
        set_property(CACHE RTTR_OPTIMIZATION_VECTOR_EXT PROPERTY STRINGS OFF SSE2 SSE3 SSE4 AVX AVX2)
        if(RTTR_OPTIMIZATION_VECTOR_EXT)
            if(MINGW AND RTTR_OPTIMIZATION_VECTOR_EXT STREQUAL "SSE")
                # MinGW has a check for a bug(?) in GCC >=6 which warns/errors on "-msse -mfpmath=sse" which leads to an invalid __FLT_EVAL_METHOD__
                message(WARNING "Due to an issue with GCC under MinGW 'SSE' is not supported. Using 'SSE2'")
                set(RTTR_OPTIMIZATION_VECTOR_EXT SSE2)
            endif()
            CheckAndAddFlag(-mfpmath=sse)
            string(TOLOWER ${RTTR_OPTIMIZATION_VECTOR_EXT} vectorExt)
            CheckAndAddFlag(-m${vectorExt})
        endif()
    endif()

    set(RTTR_OPTIMIZATION_TUNE "${RTTR_OPTIMIZATION_TUNE_DEFAULT}" CACHE STRING "Target architecture to tune for (e.g. prescott, core2, nehalem (>=Core ix), westmere)")

    # Don't use this for GCC < 8 on apple due to ICE: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=78380
    if(NOT (APPLE AND CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8))
        # Need to check this because Clang-9 with GLIBC > 1.31 generates undefined references to buildins
        # https://bugs.llvm.org/show_bug.cgi?id=45034
        include(CheckCXXSourceCompiles)
        set(CMAKE_REQUIRED_FLAGS -ffastmath)
        check_cxx_source_compiles("
#include <cmath>

int main(int argc, char** argv){
  return static_cast<int>(std::pow(2., argc));
}"      FAST_MATH_SUPPORTED)
        unset(CMAKE_REQUIRED_FLAGS)
        if(FAST_MATH_SUPPORTED)
            add_compile_options($<$<COMPILE_LANGUAGE:CXX>:${flag}>)
        endif()
    endif()

    if(RTTR_OPTIMIZATION_TUNE)
        CheckAndAddFlag(-mtune=${RTTR_OPTIMIZATION_TUNE})
    endif()

    if(APPLE)
        # ppc only?
        if(CMAKE_OSX_ARCHITECTURES MATCHES "^ppc[^;]*$")
            CheckAndAddFlags(-faltivec -maltivec) # Clang and GCC version
        endif()
    endif()
endif()
