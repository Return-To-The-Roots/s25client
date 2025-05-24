// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <s25util/warningSuppression.h>

/// Define this to 1 if you want assertions enabled
#ifndef RTTR_ENABLE_ASSERTS
#    ifdef NDEBUG
#        define RTTR_ENABLE_ASSERTS 0
#    else
#        define RTTR_ENABLE_ASSERTS 1
#    endif
#endif // !RTTR_ENABLE_ASSERTS

#ifdef _MSC_VER
extern void __cdecl __debugbreak();
#    define RTTR_BREAKPOINT __debugbreak()
#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#    define RTTR_BREAKPOINT __asm__ __volatile__("int $3\n\t")
#elif !defined(WIN32)
#    include <csignal>
#    define RTTR_BREAKPOINT raise(SIGTRAP)
#else
#    define RTTR_BREAKPOINT
#endif

[[noreturn]] void RTTR_AssertFailure(const char* condition, const char* file, int line, const char* function);
/// If true(default), a breakpoint is triggered on assert (if available)
/// Note: This breakpoint can be globally disabled by setting the environment variable
///       RTTR_DISABLE_ASSERT_BREAKPOINT to "1" or "yes" which overrides this setting
bool RTTR_IsBreakOnAssertFailureEnabled();
bool RTTR_SetBreakOnAssertFailure(bool enabled);

/* Some aspects about RTTR_Assert:
    - do-while(false) so it can be used in conditions: if(foo) RTTR_Assert(bar);
    - Don't forget parantheses around cond for cases like: RTTR_Assert(true||false);
    - Use sizeof for disabled assert to avoid unused value warnings and actual code generation
    - RTTR_AssertNoThrow which does just logging and triggers a breakpoint but does not throw (e.g. for dtors)
 */
#if RTTR_ENABLE_ASSERTS
#    define RTTR_Assert(cond)                                            \
        do                                                               \
        {                                                                \
            RTTR_IGNORE_UNREACHABLE_CODE                                 \
            if(!(cond)) /* NOLINT(readability-simplify-boolean-expr)*/   \
            {                                                            \
                if(RTTR_IsBreakOnAssertFailureEnabled())                 \
                {                                                        \
                    RTTR_BREAKPOINT;                                     \
                }                                                        \
                RTTR_AssertFailure(#cond, __FILE__, __LINE__, __func__); \
            }                                                            \
            RTTR_POP_DIAGNOSTIC                                          \
        } while(false)
#else
#    define RTTR_Assert(cond)   \
        do                      \
        {                       \
            (void)sizeof(cond); \
        } while(false)
#endif

#define RTTR_Assert_Msg(cond, msg) RTTR_Assert((cond) && (msg))
