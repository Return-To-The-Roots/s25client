// $Id: main.h 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

#pragma once

///////////////////////////////////////////////////////////////////////////////
// System-Header

#define _CRTDBG_MAP_ALLOC

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ws2tcpip.h>
#include <shlwapi.h>
#ifndef __CYGWIN__
#include <conio.h>
#endif


#ifdef _MSC_VER
#include <crtdbg.h>
#else
#include <assert.h>
#endif

#undef PlaySound

#include "win32_nanosleep.h"
#else
#include <unistd.h>
#include <stdarg.h>
#include <signal.h>
#include <dirent.h>
#include <dlfcn.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <assert.h>

#include "strlwr.h"
#endif // !_WIN32

#if defined _WIN32 && defined _DEBUG
#include <crtdbg.h>
#endif // _WIN32 && _DEBUG

#include <errno.h>
#include <cstdlib>
#include <csignal>
#include <cstdio>
#include <ctime>
#include <cmath>

#include <map>
#include <algorithm>
#include <vector>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

#include <bzlib.h>

extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#endif // !__APPLE__

#ifdef _WIN32

#ifdef _MSC_VER
#define getch _getch
#ifndef snprintf
#define snprintf _snprintf
#endif
#ifndef assert
#define assert _ASSERT
#endif
#endif

typedef int socklen_t;
#define BREAKPOINT ;
#else
#define BREAKPOINT raise(SIGTRAP)
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define HINSTANCE void*

#define closesocket close
#define LoadLibrary(x) dlopen(x, RTLD_LAZY)
#define LoadLibraryW LoadLibrary
#define LoadLibraryA LoadLibrary
#define GetProcAddress(x, y) dlsym(x, y)
#define GetProcAddressW GetProcAddress
#define GetProcAddressA GetProcAddress
#define FreeLibrary(x) dlclose(x)
#endif // _WIN32

///////////////////////////////////////////////////////////////////////////////
// Eigene Header
#include <build_paths.h>
#include <build_version.h>
#include "../libutil/src/libutil.h"
#include "../mygettext/src/mygettext.h"
#include "../liblobby/src/liblobby.h"
#include "../libsiedler2/src/libsiedler2.h"
#include "../libendian/src/libendian.h"

#include "macros.h"
#include "list.h"
#include "Swap.h"

#include "glAllocator.h"
#include "../driver/src/Sound.h"

typedef struct Rect
{
    unsigned short left, top, right, bottom;
} Rect;

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
inline T min(T a, T b) { return ((a) < (b)) ? (a) : (b); }

#undef max
template <typename T>
inline T max(T a, T b) { return ((a) < (b)) ? (b) : (a); }

/// Berechnet Differenz von 2 (unsigned!) Werten
template <typename T>
inline T SafeDiff(T a, T b) { return ((a) > (b)) ? (a - b) : (b - a); }

// 2D-Punkt
template <typename T>
struct Point
{
    T x, y;
    Point() {}
    Point(const T x, const T y) : x(x), y(y) {}
    bool operator==(const Point<T> second) const
    { return (x == second.x && y == second.y); }
};

const unsigned oo = 0xffffffff;

#endif // MAIN_H_INCLUDED
