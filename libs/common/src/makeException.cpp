// Copyright (C) 2019 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "makeException.h"
#include <boost/system/config.hpp>
#ifdef BOOST_WINDOWS_API
#    include <windows.h>
#elif defined(BOOST_POSIX_API)
#    include <cerrno>
#else
#    error "Must define windows or posix API"
#endif

std::error_code GetLastErrorCode()
{
#ifdef BOOST_WINDOWS_API
    return std::error_code(GetLastError(), std::system_category());
#else
    return std::error_code(errno, std::system_category());
#endif
}
