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

// This test the implementation of the helpers

#include "defines.h" // IWYU pragma: keep
#include "helpers/traits.h"

#include <boost/container/flat_set.hpp>
#include <boost/static_assert.hpp>
#include <vector>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

// Use unnamed namespace
namespace{

    //////////////////////////////////////////////////////////////////////////
    // EraseReturnsIterator
    struct EraseVoid{
        typedef int iterator;
        typedef const int const_iterator;
        void erase(iterator);
    };
    struct EraseIt{
        typedef int iterator;
        typedef const int const_iterator;
        iterator erase(const_iterator);
    };
    struct EraseItInherited: public EraseIt{};

    BOOST_STATIC_ASSERT(!helpers::detail::EraseReturnsIterator<EraseVoid>::value);
    BOOST_STATIC_ASSERT(helpers::detail::EraseReturnsIterator<EraseIt>::value);
    BOOST_STATIC_ASSERT(helpers::detail::EraseReturnsIterator<EraseItInherited>::value);
    BOOST_STATIC_ASSERT(helpers::detail::EraseReturnsIterator< std::vector<int> >::value);
    BOOST_STATIC_ASSERT(helpers::detail::EraseReturnsIterator< boost::container::flat_set<int> >::value);

}
