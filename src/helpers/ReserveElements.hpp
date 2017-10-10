// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef ReserveElements_h__
#define ReserveElements_h__

#include "helpers/traits.h"

namespace helpers {

/// Reserves space in a collection if possible
template<class T, bool T_hasReserve = has_member_function_reserve<void (T::*)(size_t)>::value>
struct ReserveElements
{
    static void reserve(T& collection, unsigned size) { collection.reserve(size); }
};

template<class T>
struct ReserveElements<T, false>
{
    static void reserve(T& /*collection*/, unsigned /*size*/) {}
};

} // namespace helpers

#endif // ReserveElements_h__
