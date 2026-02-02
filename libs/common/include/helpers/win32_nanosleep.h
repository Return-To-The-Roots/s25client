// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <ctime> // IWYU pragma: exports
#if defined _WIN32 && !defined __MINGW32__

#    ifndef HAVE_STRUCT_TIMESPEC
struct timespec
{
    time_t tv_sec; // Seconds.
    long tv_nsec;  // Nanoseconds.
};
#    endif

/// nanosleep replacement for windows.
int nanosleep(const struct timespec* requested_delay, struct timespec* remaining_delay);

#endif // _WIN32
