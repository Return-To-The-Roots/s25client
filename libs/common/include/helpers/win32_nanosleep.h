// Copyright (c) 2005 - 2019 Settlers Freaks (sf-team at siedler25.org)
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
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#ifndef libs_common_include_helpers_win32_nanosleep_h
#define libs_common_include_helpers_win32_nanosleep_h

#include <ctime> // IWYU pragma: exports

#ifdef _WIN32

#ifndef HAVE_STRUCT_TIMESPEC
struct timespec
{
    time_t tv_sec; // Seconds.
    long tv_nsec;  // Nanoseconds.
};
#endif // !HAVE_STRUCT_TIMESPEC

/// nanosleep replacement for windows.
int nanosleep(const struct timespec* requested_delay, struct timespec* remaining_delay);

#endif // _WIN32

#endif // !libs_common_include_helpers_win32_nanosleep_h
