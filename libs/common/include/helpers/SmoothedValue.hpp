// Copyright (c) 2018 - 2018 Settlers Freaks (sf-team at siedler25.org)
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
