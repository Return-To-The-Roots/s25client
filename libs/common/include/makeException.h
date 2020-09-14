// Copyright (c) 2019 - 2019 Settlers Freaks (sf-team at siedler25.org)
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
