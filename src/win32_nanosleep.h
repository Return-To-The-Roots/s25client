// $Id: win32_nanosleep.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef WIN32_NANOSLEEP_H_INCLUDED
#define WIN32_NANOSLEEP_H_INCLUDED

#pragma once

#ifdef _WIN32
typedef unsigned int useconds_t;

#ifndef _TIMESPEC_DEFINED
#define _TIMESPEC_DEFINED
typedef struct timespec
{
    unsigned int tv_sec;    // Seconds.
    long int tv_nsec;       // Nanoseconds.
} timespec_t;
#endif

/// Sleep at least some number of microseconds.
int usleep (useconds_t microseconds);

/// nanosleep replacement for windows.
int nanosleep(const timespec_t* requested_delay, timespec_t* remaining_delay);

#endif // _WIN32

#endif // !WIN32_NANOSLEEP_H_INCLUDED
