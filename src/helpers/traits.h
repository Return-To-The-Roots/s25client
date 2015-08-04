// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your oposion) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#include <boost/tti/has_member_function.hpp>

namespace helpers{

    /// Removes a pointer declaration from a type
    /// int* -> int; int ->int; int** -> int*
    template<typename T>
    struct remove_pointer
    {
        typedef T type;
    };
    template<typename T>
    struct remove_pointer<T*>
    {
        typedef T type;
    };


    /// Checks if the given type as a member function called reserve
    BOOST_TTI_HAS_MEMBER_FUNCTION(reserve)
    /// Checks if the given type as a member function called push_back
    BOOST_TTI_HAS_MEMBER_FUNCTION(push_back)
}