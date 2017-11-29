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

#pragma once
#ifndef commonDefines_h__
#define commonDefines_h__

// IWYU pragma: begin_exports

#ifndef _CRTDBG_MAP_ALLOC
#define _CRTDBG_MAP_ALLOC
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
// Allow e.g. UINT64_C macros
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif

#ifdef _WIN32

// Enable Memory Leak Detection
//#define RTTR_CRTDBG

// Enable catching of exceptions
//#define RTTR_HWETRANS

#define WIN32_LEAN_AND_MEAN
#ifdef _MSC_VER
// Visual Studio
#ifdef _DEBUG
#include <crtdbg.h>
#endif              // _DEBUG
#include <stdlib.h> // Required for crtdbg.h
#if !defined(snprintf) && _MSC_VER < 1900
#define snprintf _snprintf
#endif
extern void __cdecl __debugbreak();
#define RTTR_BREAKPOINT __debugbreak()
#else
// Not Visual Studio
#define RTTR_BREAKPOINT
#endif

typedef int socklen_t;
#else
// Non-Windows
#if(defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__)))
#define RTTR_BREAKPOINT __asm__ __volatile__("int $3\n\t")
#else
#include <csignal>
#define RTTR_BREAKPOINT raise(SIGTRAP)
#endif
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define HINSTANCE void*

#define closesocket close
#define LoadLibrary(lib) dlopen(lib, RTLD_LAZY)
#define LoadLibraryW LoadLibrary
#define LoadLibraryA LoadLibrary
#define GetProcAddress(lib, name) dlsym(lib, name)
#define GetProcAddressW GetProcAddress
#define GetProcAddressA GetProcAddress
#define FreeLibrary(lib) dlclose(lib)
#endif // !_WIN32

#include "RTTR_Assert.h"

// Include to use e.g. boost macros like BOOST_CONSTEXPR
#include <boost/config.hpp>
// Fixed width types like uint32_t shall be treated like build-in types
#include <stdint.h>

// IWYU pragma: end_exports

// Macro that can be used to suppress unused warnings. Required e.g. for const boost::arrays defined in headers
// Don't use this if not absolutely necessary!
#ifdef __GNUC__
#define SUPPRESS_UNUSED __attribute__((unused))
#else
#define SUPPRESS_UNUSED
#endif

// RTTR_FUNCTION_NAME evaluates to the name of the current function if supported
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) /* C99 supports __func__ as a standard. */
#define RTTR_FUNCTION_NAME __func__
#elif((__GNUC__ >= 2) || defined(_MSC_VER))
#define RTTR_FUNCTION_NAME __FUNCTION__
#else
#define RTTR_FUNCTION_NAME "<Unknown Func>"
#endif

#if defined _WIN32 && defined _DEBUG && defined _MSC_VER && defined RTTR_CRTDBG
// Check for heap corruption
#define CHECK_HEAP_CORRUPTION _ASSERTE(_CrtCheckMemory());
#else
#define CHECK_HEAP_CORRUPTION
#endif // _WIN32 && _DEBUG && RTTR_CRTDBG

/// Call a member function trough an object and a member function pointer
#define CALL_MEMBER_FN(object, ptrToMember) ((object).*(ptrToMember))

/// Deletes the ptr and sets it to NULL
template<typename T>
inline void deletePtr(T*& ptr)
{
    delete ptr;
    ptr = 0;
}

/// Calculate |a-b| of 2 unsigned values
template<typename T>
inline T safeDiff(T a, T b)
{
    return (a > b) ? a - b : b - a;
}

/// Same as static_cast<T> but assert that it actually can be casted via dynamic_cast
template<typename T, typename T_Src>
inline T checkedCast(T_Src src)
{
    RTTR_Assert(dynamic_cast<T>(src));
    return static_cast<T>(src);
}

// Fwd decl
namespace boost {
namespace filesystem {
}
namespace nowide {
}
} // namespace boost

/// Shortcut for boost::filesystem
namespace bfs = boost::filesystem;
/// Shortcut for boost::nowide
namespace bnw = boost::nowide;

// Suppress uninitialized v_
//-V:BOOST_SCOPED_ENUM_DECLARE_BEGIN:730,801

#endif // commonDefines_h__
