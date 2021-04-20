// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/type_traits/make_void.hpp>
#include <iterator>
#include <utility>

namespace helpers {

/// Returns the most efficient insert operator
template<class T, typename = void>
struct GetInsertIterator
{
    static auto get(T& collection) { return std::inserter(collection, collection.end()); }
};

template<class T>
struct GetInsertIterator<T,
                         boost::void_t<decltype(std::declval<T>().push_back(std::declval<typename T::value_type>()))>>
{
    static auto get(T& collection) { return std::back_inserter(collection); }
};

} // namespace helpers
