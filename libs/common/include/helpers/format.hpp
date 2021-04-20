// Copyright (c) 2019 - 2019 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
