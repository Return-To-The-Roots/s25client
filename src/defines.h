// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef defines_h__
#define defines_h__

// IWYU pragma: begin_exports

#ifndef _CRTDBG_MAP_ALLOC
#   define _CRTDBG_MAP_ALLOC
#endif
#ifndef NOMINMAX
#   define NOMINMAX
#endif

#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#   ifdef _MSC_VER
#       include <stdlib.h> // Required for crtdbg.h
#       include <crtdbg.h>
#       if !defined(snprintf) && _MSC_VER < 1900
#           define snprintf _snprintf
#       endif
        extern void __cdecl __debugbreak();
#       define BREAKPOINT __debugbreak()
#       ifndef assert
#           define assert _ASSERT
#       endif
#   else
#       include <assert.h>
#       define BREAKPOINT
#   endif

#   undef PlaySound
    typedef int socklen_t;
#else
#   include <csignal>
#   define BREAKPOINT raise(SIGTRAP)
#   define SOCKET int
#   define INVALID_SOCKET -1
#   define SOCKET_ERROR -1
#   define HINSTANCE void*

#   define closesocket close
#   define LoadLibrary(lib) dlopen(lib, RTLD_LAZY)
#   define LoadLibraryW LoadLibrary
#   define LoadLibraryA LoadLibrary
#   define GetProcAddress(lib, name) dlsym(lib, name)
#   define GetProcAddressW GetProcAddress
#   define GetProcAddressA GetProcAddress
#   define FreeLibrary(lib) dlclose(lib)
#   include <cassert>
#endif // !_WIN32

#ifndef NO_BUILD_PATHS
#   include <build_paths.h>
#endif

#include "macros.h"
#include "RTTR_Assert.h"

// Include to use e.g. boost macros like BOOST_CONSTEXPR
#include <boost/config.hpp>

// IWYU pragma: end_exports

///////////////////////////////////////////////////////////////////////////////
/**
 *  konvertiert einen void*-Pointer zu einem function-Pointer mithilfe einer
 *  Union. GCC meckert da sonst wegen "type punned pointer" bzw
 *  "iso c++ forbids conversion".
 *
 *  @author FloSoft
 */
template <typename F>
inline F pto2ptf(void* o)
{
    union
    {
        F f;
        void* o;
    } U;
    U.o = o;

    return U.f;
}

#undef min
template <typename T>
inline T min(T a, T b) { return (a < b) ? a : b; }

#undef max
template <typename T>
inline T max(T a, T b) { return (a < b) ? b : a; }

/// Berechnet Differenz von 2 (unsigned!) Werten
template <typename T>
inline T SafeDiff(T a, T b) { return (a > b) ? a - b : b - a; }

/// Deletes the ptr and sets it to NULL
template <typename T>
inline void deletePtr(T*& ptr)
{
    delete ptr;
    ptr = 0;
}

// Fwd decl
namespace boost{namespace filesystem{}}

/// Shortcut for boost::filesystem
namespace bfs = boost::filesystem;

#endif // defines_h__

