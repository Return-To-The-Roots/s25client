// Copyright (C) 2018 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/circular_buffer.hpp>
#include <numeric>

namespace helpers {
/// A value that is smoothed over the past N values added
template<typename T>
class SmoothedValue
{
    boost::circular_buffer<T> pastValues_;

public:
    explicit SmoothedValue(size_t maxValues) : pastValues_(maxValues) {}
    void add(const T& value) { pastValues_.push_back(value); }
    T get() const
    {
        if(pastValues_.empty())
            return T();
        else
            return std::accumulate(pastValues_.begin(), pastValues_.end(), T(0)) / static_cast<T>(size());
    }
    size_t size() const { return pastValues_.size(); }
};
} // namespace helpers
