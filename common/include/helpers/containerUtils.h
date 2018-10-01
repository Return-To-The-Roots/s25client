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

///////////////////////////////////////////////////////////////////////////////

#ifndef containerUtils_h__
#define containerUtils_h__

#include "traits.h"
#include <boost/foreach.hpp>
#include <algorithm>

namespace helpers {

namespace detail {
    template<class T, EEraseIterValidy::Type T_EraseIterValidy = EraseIterValidy<T>::value>
    struct EraseImpl;

    template<class T>
    struct EraseImpl<T, EEraseIterValidy::IterReturned>
    {
        typedef typename T::iterator iterator;
        typedef typename T::const_iterator const_iterator;
        static iterator erase(T& container, iterator it) { return container.erase(it); }
    };
    template<class T>
    struct EraseImpl<T, EEraseIterValidy::NextValid>
    {
        // This one gets used for e.g. std::set whos erase does not return
        // an iterator until C++11
        typedef typename T::iterator iterator;
        typedef typename T::const_iterator const_iterator;
        static iterator erase(T& container, iterator it)
        {
            container.erase(it++);
            return it;
        }
    };

    template<class T>
    struct EraseImpl<T, EEraseIterValidy::PrevValid>
    {
        typedef typename T::iterator iterator;
        typedef typename T::const_iterator const_iterator;
        static iterator erase(T& container, iterator it)
        {
            // If only previous iterators remain valid, store the predecessor
            // and return this after incrementing it after the erase
            // Corner case: If we erase the first element there is no previous one and we return begin()
            bool isBegin = it == container.begin();
            iterator tmp = it;
            if(!isBegin)
                --tmp;
            container.erase(it);
            if(isBegin)
                tmp = container.begin();
            else
                ++tmp;
            return tmp;
        }
    };

    template<class T, bool T_hasPopFront = helpers::has_member_function_pop_front<T, void>::value>
    struct PopFrontImpl
    {
        static void pop(T& container) { container.pop_front(); }
    };
    template<class T>
    struct PopFrontImpl<T, false>
    {
        static void pop(T& container) { container.erase(container.begin()); }
    };

    template<class T_Container, class T_Type,
             bool T_hasKeyType = HasAnyMemberCalled_key_type<T_Container>::value&& HasAnyMemberCalled_find<T_Container>::value>
    struct HasCorrectFindMember
    {
        enum
        {
            value = boost::is_convertible<T_Type, typename T_Container::key_type>::value
        };
    };

    template<class T_Container, class T_Type>
    struct HasCorrectFindMember<T_Container, T_Type, false>
    {
        enum
        {
            value = false
        };
    };

    template<class T, class U, bool T_useFind = HasCorrectFindMember<T, U>::value>
    struct FindImpl
    {
        static typename GetIteratorType<T>::type find(T& container, const U& value) { return container.find(value); }
    };

    template<class T, class U>
    struct FindImpl<T, U, false>
    {
        static typename GetIteratorType<T>::type find(T& container, const U& value)
        {
            return std::find(container.begin(), container.end(), value);
        }
    };
} // namespace detail

/// Removes an element from a container by its iterator and returns an iterator to the next element
/// Works only for list and set as they don't invalidate other iterators, so erase is save to call inside a loop
/// Works also for reverse iterators
template<typename T>
inline typename T::iterator erase(T& container, typename T::iterator it)
{
    return detail::EraseImpl<T>::erase(container, it);
}

template<typename T>
inline typename T::reverse_iterator erase(T& container, typename T::reverse_iterator it)
{
    typename T::reverse_iterator tmp = it;
    return typename T::reverse_iterator(erase(container, (++tmp).base()));
}

/// Removes the first element in a container
template<typename T>
inline void pop_front(T& container)
{
    RTTR_Assert(!container.empty());
    detail::PopFrontImpl<T>::pop(container);
}

/// Effective implementation of find. Uses the containers find function if available
template<typename T, typename U>
inline typename GetIteratorType<T>::type find(T& container, const U& value)
{
    return detail::FindImpl<T, U>::find(container, value);
}

/// Returns true if the container contains the given value
/// Uses the find member function if applicable otherwise uses the std::find method
template<typename T, typename U>
inline bool contains(const T& container, const U& value)
{
    return find(container, value) != container.end();
}

/// Returns true if the container contains a value matching the predicate
template<typename T, class T_Predicate>
inline bool containsPred(const T& container, const T_Predicate& predicate)
{
    return std::find_if(container.begin(), container.end(), predicate) != container.end();
}

/// Remove duplicate values from the given container without changing the order
template<class T>
inline void makeUnique(T& container)
{
    // Containers with less than 2 elements are always unique
    if(container.size() < 2u)
        return;
    typename T::iterator itInsert = container.begin();
    // We always begin inserting at 2nd pos so skip first
    ++itInsert;
    // And now start inserting all values starting from the 2nd
    for(typename T::iterator it = itInsert; it != container.end(); ++it)
    {
        // If current element is not found in [begin, insertPos) then add it at insertPos and inc insertPos
        if(std::find(container.begin(), itInsert, *it) == itInsert)
            *(itInsert++) = *it;
    }
    container.erase(itInsert, container.end());
}

/// Returns the index of the given element in the container or -1 when not found
/// Note: Only works for containers with less than 2^31 - 1 elements.
template<class T_Container, class T_Element>
inline int indexOf(const T_Container& container, const T_Element element)
{
    int index = 0;
    BOOST_FOREACH(const typename T_Container::value_type& curEl, container)
    {
        if(curEl == element)
            return index;
        ++index;
    }
    return -1;
}

} // namespace helpers

#endif // containerUtils_h__
