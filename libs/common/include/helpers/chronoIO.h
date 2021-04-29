// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
