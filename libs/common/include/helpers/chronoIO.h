// Copyright (c) 2018 - 2019 Settlers Freaks (sf-team at siedler25.org)
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

#include <chrono>
#include <iosfwd>
#include <memory>

/// Allow printing of std::duration values to streams
/// See also C++20 operator<<(std::chrono::duration)
namespace helpers {
#define RTTR_DEF_GETTIMEUNIT(R, RES) \
    constexpr const char* getTimeUnit(R) { return RES; }
RTTR_DEF_GETTIMEUNIT(std::atto, "as")
RTTR_DEF_GETTIMEUNIT(std::femto, "fs")
RTTR_DEF_GETTIMEUNIT(std::pico, "ps")
RTTR_DEF_GETTIMEUNIT(std::nano, "ns")
RTTR_DEF_GETTIMEUNIT(std::micro, "us")
RTTR_DEF_GETTIMEUNIT(std::milli, "ms")
RTTR_DEF_GETTIMEUNIT(std::chrono::seconds::period, "s")
RTTR_DEF_GETTIMEUNIT(std::chrono::minutes::period, "min")
RTTR_DEF_GETTIMEUNIT(std::chrono::hours::period, "h")
#undef RTTR_DEF_GETTIMEUNIT

template<class T>
struct WithUnit;

template<class T, class R>
struct WithUnit<std::chrono::duration<T, R>>
{
    std::chrono::duration<T, R> v;

    friend std::ostream& operator<<(std::ostream& os, const WithUnit& v)
    {
        return os << v.v.count() << ::helpers::getTimeUnit(R{});
    }
};

/// Create a wrapper type that adds the SI unit when streamed
template<class T>
auto withUnit(const T& v)
{
    return WithUnit<T>{v};
}
} // namespace helpers
