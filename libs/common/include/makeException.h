// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/strUtils.h"
#include <stdexcept>
#include <system_error>

/// Create an exception by concatenating the passed args
template<typename T_Exception = std::runtime_error, typename... Args>
auto makeException(Args&&... args)
{
    return T_Exception(helpers::concat(std::forward<Args>(args)...));
}

/// Return the last error (either GetLastError or errno)
std::error_code GetLastErrorCode();

template<typename... Args>
std::system_error makeLastSystemError(Args&&... args)
{
    return {GetLastErrorCode(), helpers::concat(std::forward<Args>(args)...)};
}
