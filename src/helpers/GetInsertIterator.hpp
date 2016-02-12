// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef GetInsertIterator_h__
#define GetInsertIterator_h__

#include "helpers/traits.h"

namespace helpers{

    /// Returns the most efficient insert operator defining its type as "iterator"
    template<class T, bool T_hasPushBack = has_member_function_push_back<void (T::*)(const typename T::value_type&)>::value>
    struct GetInsertIterator
    {
        typedef std::back_insert_iterator<T> iterator;
        static iterator get(T& collection)
        {
            return iterator(collection);
        }
    };

    template<class T>
    struct GetInsertIterator<T, false>
    {
        typedef std::insert_iterator<T> iterator;
        static iterator get(T& collection)
        {
            return iterator(collection, collection.end());
        }
    };

}

#endif // GetInsertIterator_h__
