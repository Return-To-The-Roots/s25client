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

#include <boost/format.hpp>

namespace helpers {
template<typename... T>
// NOLINTNEXTLINE(performance-unnecessary-value-param)
std::string format(boost::format fmt, T&&... args)
{
    // Pre C++17 expansion (fmt % arg1 % arg2)
    using expander = int[];
    (void)expander{0, (fmt % std::forward<T>(args), 0)...};
    return std::move(fmt).str();
}
template<typename... T>
std::string format(const char* fmtString, T&&... args)
{
    return format(boost::format(fmtString), std::forward<T>(args)...);
}
template<typename... T>
std::string format(const std::string& fmtString, T&&... args)
{
    return format(boost::format(fmtString), std::forward<T>(args)...);
}
} // namespace helpers
