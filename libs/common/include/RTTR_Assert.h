// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#ifndef RTTRAssert_h__
#define RTTRAssert_h__

/// Define this to 1 if you want assertions enabled
#ifndef RTTR_ENABLE_ASSERTS
#ifdef NDEBUG
#define RTTR_ENABLE_ASSERTS 0
#else
#define RTTR_ENABLE_ASSERTS 1
#endif
#endif // !RTTR_ENABLE_ASSERTS

#ifdef _MSC_VER
extern void __cdecl __debugbreak();
#define RTTR_BREAKPOINT __debugbreak()
#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#define RTTR_BREAKPOINT __asm__ __volatile__("int $3\n\t")
#elif !defined(WIN32)
#include <csignal>
#define RTTR_BREAKPOINT raise(SIGTRAP)
#else
#define RTTR_BREAKPOINT
#endif

void RTTR_AssertFailure(const char* condition, const char* file, int line, const char* function, bool throwException = true);
bool RTTR_IsBreakOnAssertFailureEnabled();
/// If true(default), a breakpoint is triggered on assert (if available)
/// Note: This breakpoint can be globally disabled by setting the environment variable
///       RTTR_DISABLE_ASSERT_BREAKPOINT to "1" or "yes" which overrides this setting
extern bool RTTR_AssertEnableBreak;

/* Some aspects about RTTR_Assert:
    - do-while(false) so it can be used in conditions: if(foo) RTTR_Assert(bar);
    - Don't forget parantheses around cond for cases like: RTTR_Assert(true||false);
    - Use sizeof for disabled assert to avoid unused value warnings and actual code generation
    - RTTR_AssertNoThrow which does just logging and triggers a breakpoint but does not throw (e.g. for dtors)
 */
#if RTTR_ENABLE_ASSERTS
#define RTTR_Assert(cond)                                            \
    do                                                               \
    {                                                                \
        if(!(cond))                                                  \
        {                                                            \
            if(RTTR_IsBreakOnAssertFailureEnabled())                 \
            {                                                        \
                RTTR_BREAKPOINT;                                     \
            }                                                        \
            RTTR_AssertFailure(#cond, __FILE__, __LINE__, __func__); \
        }                                                            \
    } while(false)
#define RTTR_AssertNoThrow(cond)                                            \
    do                                                                      \
    {                                                                       \
        if(!(cond))                                                         \
        {                                                                   \
            if(RTTR_IsBreakOnAssertFailureEnabled())                        \
            {                                                               \
                RTTR_BREAKPOINT;                                            \
            }                                                               \
            RTTR_AssertFailure(#cond, __FILE__, __LINE__, __func__, false); \
        }                                                                   \
    } while(false)
#else
#define RTTR_Assert(cond)   \
    do                      \
    {                       \
        (void)sizeof(cond); \
    } while(false)
#define RTTR_AssertNoThrow RTTR_Assert
#endif

#endif // RTTRAssert_h__
