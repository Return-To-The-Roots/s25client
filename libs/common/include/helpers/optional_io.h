// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/test/tools/detail/print_helper.hpp>
#include <optional>
#include <ostream>

template<class CharType, class CharTrait, class T>
inline std::basic_ostream<CharType, CharTrait>& operator<<(std::basic_ostream<CharType, CharTrait>& out,
                                                           std::optional<T> const& v)
{
    return out << (v ? *v : "--");
}
namespace boost::test_tools::tt_detail {
template<typename T>
struct print_log_value<std::optional<T>>
{
    void operator()(std::ostream& os, std::optional<T> const& v)
    {
        if(v)
            os << *v;
        else
            os << "<nullopt>";
    }
};
} // namespace boost::test_tools::tt_detail