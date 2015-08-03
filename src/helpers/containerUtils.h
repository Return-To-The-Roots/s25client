// $Id: nobHarborBuilding.cpp 9546 2014-12-14 12:06:35Z marcus $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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

///////////////////////////////////////////////////////////////////////////////

#ifndef containerUtils_h__
#define containerUtils_h__

#include <algorithm>
#include <list>
#include <set>

namespace helpers{
    template<class T>
    struct EraseFromContainer;

    template<typename T>
    struct EraseFromContainer< std::list<T> >
    {
        typedef std::list<T> list;
        typedef typename list::iterator iterator;
        typedef typename list::const_iterator const_iterator;

        static iterator erase(list& container, const iterator& it) {
            return container.erase(it);
        }
        static const_iterator erase(list& container, const const_iterator& it) {
            return container.erase(it);
        }
    };

    template<typename T, typename U>
    struct EraseFromContainer< std::set<T, U> >
    {
        typedef std::set<T, U> set;
        typedef typename set::iterator iterator;
        typedef typename set::const_iterator const_iterator;

        static iterator erase(set& container, const iterator& it) {
            // Sets do not support returning the next iterator until C++11
            iterator tmp = it;
            container.erase(tmp++);
            return tmp;
        }
    };

    /// Removes an element from a container by its iterator and returns an iterator to the next element
    /// Works only for list and set as they don't invalidate other iterators, so erase is save to call inside a loop
    /// Works also for reverse iterators
    template<typename T>
    inline typename T::iterator erase(T& container, const typename T::iterator& it)
    {
        return EraseFromContainer<T>::erase(container, it);
    }

    /*template<typename T>
    inline typename T::const_iterator erase(T& container, const typename T::const_iterator& it)
    {
        return EraseFromContainer<T>::erase(it);
    }*/

    template<typename T>
    inline typename T::reverse_iterator erase(T& container, const typename T::reverse_iterator& it)
    {
        typename T::reverse_iterator tmp = it;
        return typename T::reverse_iterator(erase(container, (++tmp).base()));
    }

    /*template<typename T>
    inline typename T::const_reverse_iterator erase(T& container, const typename T::const_reverse_iterator& it)
    {
        typename std::set<T>::const_reverse_iterator tmp = it;
        return typename T::const_reverse_iterator(erase(container, (++tmp).base()));
    }*/

    /// Removes the first element in a container
    template<typename T>
    inline void pop_front(T& container)
    {
        assert(!container.empty());
        container.pop_front();
    }

    template<typename T, typename U>
    inline void pop_front(std::set<T, U>& container)
    {
        assert(!container.empty());
        container.erase(container.begin());
    }

    /// Returns true if the container contains the given value
    template<typename T, typename U>
    bool contains(const T& container, const U& value)
    {
        return std::find(container.begin(), container.end(), value) != container.end();
    }

} // namespace helpers

#endif // containerUtils_h__