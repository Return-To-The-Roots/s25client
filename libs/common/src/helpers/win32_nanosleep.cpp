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

#include "helpers/win32_nanosleep.h"

#ifdef _WIN32
#    include <windows.h>
#    include <cstdint>

/**
 *  nanosleep replacement for windows.
 */
int nanosleep(const struct timespec* requested_delay, struct timespec* /*remaining_delay*/)
{
    const int64_t usPerSecond = 1000 * 1000;
    const int64_t nsPerus = 1000;
    int64_t micro_delay = static_cast<int64_t>(requested_delay->tv_sec) * usPerSecond
                          + (requested_delay->tv_nsec + nsPerus - 1) / nsPerus; // Round

    HANDLE timer = CreateWaitableTimer(nullptr, TRUE, nullptr);
    if(timer == nullptr)
        return -1;
    LARGE_INTEGER ft;
    ft.QuadPart = -(10 * micro_delay); // 100ns intervalls, negative to be relative
    int result;
    if(SetWaitableTimer(timer, &ft, 0, nullptr, nullptr, 0) && WaitForSingleObject(timer, INFINITE) == WAIT_OBJECT_0)
        result = 0;
    else
        result = -1;
    CloseHandle(timer);
    return result;
}

#endif // !_WIN32
