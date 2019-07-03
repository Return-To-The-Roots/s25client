// Copyright (c) 2005 - 2019 Settlers Freaks (sf-team at siedler25.org)
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
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#ifndef libs_common_include_helpers_GetInsertIterator_hpp
#define libs_common_include_helpers_GetInsertIterator_hpp

#include <boost/type_traits/make_void.hpp>

namespace helpers {

/// Returns the most efficient insert operator
template<class T, typename = void>
struct GetInsertIterator
{
    static auto get(T& collection) { return std::insert_iterator<T>(collection, collection.end()); }
};

template<class T>
struct GetInsertIterator<T, boost::void_t<decltype(std::declval<T>().push_back(std::declval<typename T::value_type>()))>>
{
    static auto get(T& collection) { return std::back_insert_iterator<T>(collection); }
};

} // namespace helpers

#endif // !libs_common_include_helpers_GetInsertIterator_hpp
