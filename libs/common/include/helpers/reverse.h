// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <iterator>

namespace helpers {

template<typename T>
struct ReverseAdapter
{
    T range;
};

template<typename T>
constexpr auto begin(ReverseAdapter<T> w)
{
    return std::make_reverse_iterator(w.range.end());
}

template<typename T>
constexpr auto end(ReverseAdapter<T> w)
{
    return std::make_reverse_iterator(w.range.begin());
}

/// Reverse a range: for(auto i: reverse(container))
template<typename T>
constexpr ReverseAdapter<T> reverse(T&& range)
{
    return {std::forward<T>(range)};
}
} // namespace helpers
