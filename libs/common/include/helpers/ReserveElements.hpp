// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/type_traits/make_void.hpp>
#include <utility>

namespace helpers {

/// Reserves space in a collection if possible
template<class T, typename = void>
struct ReserveElements
{
    static void reserve(T& /*collection*/, unsigned /*size*/) {}
};

template<class T>
struct ReserveElements<T, boost::void_t<decltype(std::declval<T>().reserve(0u))>>
{
    static void reserve(T& collection, unsigned size) { collection.reserve(size); }
};

} // namespace helpers
