// $Id: win32_nanosleep.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "main.h"
#include "win32_nanosleep.h"

#ifdef _WIN32

#include <winsock2.h>
#include <ctime>
#include <cstdio>

///////////////////////////////////////////////////////////////////////////////
/**
 *  Sleep at least some number of microseconds
 */
int usleep (useconds_t microseconds)
{
    int err = 0;

    if ( microseconds )
    {
        static const useconds_t one_second = 1000000;
        static SOCKET sock = SOCKET_ERROR;

        timeval tv_delay;
        fd_set set;

        if(sock == SOCKET_ERROR)
            sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

        FD_ZERO(&set);
        FD_SET(sock, &set);

        tv_delay.tv_sec = microseconds / one_second;
        tv_delay.tv_usec = microseconds % one_second;

        err = select(0, NULL, NULL, &set, &tv_delay);
    }
    return err;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  nanosleep replacement for windows.
 */
int nanosleep(const timespec_t* requested_delay, timespec_t* remaining_delay)
{
    const useconds_t one_second = 1000000;
    const useconds_t nano_per_micro = 1000;
    useconds_t micro_delay;

    micro_delay = requested_delay->tv_sec * one_second
                  + ( requested_delay->tv_nsec + nano_per_micro - 1 )
                  / nano_per_micro;

    return usleep (micro_delay);
}

#endif // !_WIN32
