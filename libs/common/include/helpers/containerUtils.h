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

#pragma once

#include <boost/type_traits/make_void.hpp>
#include <algorithm>
#include <iterator>

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
        static auto find(T& container, const U& value)
        {
            using std::begin;
            using std::end;
            return std::find(begin(container), end(container), value);
        }
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
typename T::iterator erase(T& container, typename T::iterator it)
{
    return container.erase(it);
}

template<typename T>
auto erase(T& container, typename T::reverse_iterator it)
{
    return typename T::reverse_iterator(erase(container, (++it).base()));
}

template<typename T, typename T_Element>
void remove(T& container, T_Element&& element)
{
    using std::begin;
    using std::end;
    container.erase(std::remove(begin(container), end(container), std::forward<T_Element>(element)), end(container));
}

template<typename T, typename T_Predicate>
void remove_if(T& container, T_Predicate&& predicate)
{
    using std::begin;
    using std::end;
    container.erase(std::remove_if(begin(container), end(container), std::forward<T_Predicate>(predicate)), end(container));
}

/// Removes the first element in a container
template<typename T>
void pop_front(T& container)
{
    RTTR_Assert(!container.empty());
    detail::PopFrontImpl<T>::pop(container);
}

/// Effective implementation of find. Uses the containers find function if available
template<typename T, typename U>
auto find(T& container, const U& value)
{
    return detail::FindImpl<T, U>::find(container, value);
}

template<typename T, class T_Predicate>
auto find_if(T& container, T_Predicate&& predicate)
{
    using std::begin;
    using std::end;
    return std::find_if(begin(container), end(container), std::forward<T_Predicate>(predicate));
}

/// Returns true if the container contains the given value
/// Uses the find member function if applicable otherwise uses the std::find method
template<typename T, typename U>
bool contains(const T& container, const U& value)
{
    using std::end;
    return find(container, value) != end(container);
}

/// Returns true if the container contains a value matching the predicate
template<typename T, class T_Predicate>
bool contains_if(const T& container, T_Predicate&& predicate)
{
    using std::end;
    return find_if(container, std::forward<T_Predicate>(predicate)) != end(container);
}

/// Remove duplicate values from the given sorted container
template<class T>
void makeUniqueSorted(T& container)
{
    const auto newEnd = std::unique(begin(container), end(container));
    container.erase(newEnd, end(container));
}
/// Remove duplicate values from the given container, sorts it first
template<class T>
void makeUnique(T& container)
{
    std::sort(begin(container), end(container));
    makeUniqueSorted(container);
}
template<class T, class T_Predicate>
void makeUnique(T& container, T_Predicate&& predicate)
{
    std::sort(begin(container), end(container), std::forward<T_Predicate>(predicate));
    makeUniqueSorted(container);
}
/// Remove duplicate values from the given container without changing the order
template<class T>
void makeUniqueStable(T& container)
{
    // Containers with less than 2 elements are always unique
    if(container.size() < 2u)
        return;
    auto itInsert = begin(container);
    // We always begin inserting at 2nd pos so skip first
    ++itInsert;
    // And now start inserting all values starting from the 2nd
    for(auto it = itInsert; it != container.end(); ++it)
    {
        // If current element is not found in [begin, insertPos) then add it at insertPos and inc insertPos
        if(std::find(begin(container), itInsert, *it) == itInsert)
            *(itInsert++) = std::move(*it);
    }
    container.erase(itInsert, end(container));
}

/// Returns the index of the given element in the container or -1 when not found
/// Note: Only works for containers with less than 2^31 - 1 elements.
template<class T_Container, class T_Element>
int indexOf(const T_Container& container, const T_Element& element)
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
