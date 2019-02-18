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

#include <boost/type_traits/make_void.hpp>
#include <algorithm>

namespace helpers {

namespace detail {

    template<class T, typename = void>
    struct PopFrontImpl
    {
        static void pop(T& container) { container.erase(container.begin()); }
    };
    template<class T>
    struct PopFrontImpl<T, boost::void_t<decltype(T::pop_front())>>
    {
        static void pop(T& container) { container.pop_front(); }
    };

    template<class T, class U, typename = void>
    struct FindImpl
    {
        static auto find(T& container, const U& value) { return std::find(container.begin(), container.end(), value); }
    };

    template<class T, class U>
    struct FindImpl<T, U, boost::void_t<decltype(std::declval<T>().find(std::declval<U>()))>>
    {
        static auto find(T& container, const U& value) { return container.find(value); }
    };
} // namespace detail

/// Removes an element from a container by its iterator and returns an iterator to the next element
/// Works only for list and set as they don't invalidate other iterators, so erase is save to call inside a loop
/// Works also for reverse iterators
template<typename T>
inline typename T::iterator erase(T& container, typename T::iterator it)
{
    return container.erase(it);
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
inline auto find(T& container, const U& value)
{
    return detail::FindImpl<T, U>::find(container, value);
}

template<typename T, class T_Predicate>
inline auto findPred(T& container, T_Predicate&& predicate)
{
    return std::find_if(container.begin(), container.end(), std::forward<T_Predicate>(predicate));
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
inline bool containsPred(const T& container, T_Predicate&& predicate)
{
    return findPred(container, std::forward<T_Predicate>(predicate)) != container.end();
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
    for(const auto& curEl : container)
    {
        if(curEl == element)
            return index;
        ++index;
    }
    return -1;
}

} // namespace helpers

#endif // containerUtils_h__
